#include "pch.h"
#include "MenuItems/MI_Backtrader.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>
#include <Ext/ImGuiLoadBar.h>
#include <Utils/UT_Time.h>

namespace jv::bt
{
	constexpr int32_t MAX_ZOOM = 30;
	
	enum BTMenuIndex
	{
		btmiPortfolio,
		btmiAlgorithms,
		btmiRunInfo
	};

	void MI_Backtrader::Load(STBT& stbt)
	{
		const auto tempScope = stbt.tempArena.CreateScope();

		names = MI_Symbols::GetSymbolNames(stbt);
		enabled = MI_Symbols::GetEnabled(stbt, names, enabled);

		uint32_t c = 0;
		for (uint32_t i = 0; i < names.length; i++)
			c += enabled[i];

		buffers = CreateArray<char*>(stbt.arena, c + 1);
		for (uint32_t i = 0; i < buffers.length; i++)
			buffers[i] = stbt.arena.New<char>(16);
		timeSeries = CreateArray<TimeSeries>(stbt.arena, c);

		auto namesCharPtrs = CreateArray<const char*>(stbt.tempArena, names.length);

		uint32_t index = 0;
		for (size_t i = 0; i < enabled.length; i++)
		{
			if (!enabled[i])
				continue;
			uint32_t _;
			namesCharPtrs[index] = names[i].c_str();
			timeSeries[index++] = MI_Symbols::LoadSymbol(stbt, i, names, _);			
		}

		portfolio = Portfolio::Create(stbt.arena, namesCharPtrs.ptr, names.length);
		for (uint32_t i = 0; i < c; i++)
			portfolio.stocks[i].symbol = namesCharPtrs[i];

		subIndex = 0;
		symbolIndex = -1;
		if (enabled.length > 0)
			symbolIndex = 0;
		normalizeGraph = false;

		algoIndex = -1;
		stepwise = false;
		log = false;
		pauseOnFinish = false;
		pauseOnFinishAll = true;
		running = false;
		instantMode = false;

		trades = stbt.arena.New<STBTTrade>(timeSeries.length);

		uint32_t n = 1;
		snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);
		snprintf(buffBuffer, sizeof(buffBuffer), "%i", n);
		n = MAX_ZOOM;
		snprintf(zoomBuffer, sizeof(zoomBuffer), "%i", n);
		float f = 1e-3f;
		snprintf(feeBuffer, sizeof(feeBuffer), "%f", f);
		f = 10000;
		snprintf(buffers[0], sizeof(buffers[0]), "%f", f);

		stbt.tempArena.DestroyScope(tempScope);
		subScope = stbt.arena.CreateScope();
		highlightedGraphIndex = 0;
	}

	bool MI_Backtrader::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		if (running)
			return false;

		const char* buttons[]
		{
			"Environment",
			"Algorithms",
			"Run Info"
		};

		const char* tooltips[]
		{
			"Adjust dates and\nstarting portfolio.", 
			"Select an algorithm\nto be used.", 
			"Start and adjust\nrun settings."
		};

		for (uint32_t i = 0; i < 3; i++)
		{
			TryDrawTutorialText(stbt, tooltips[i]);

			const bool selected = subIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });
			if (ImGui::Button(buttons[i]))
				subIndex = i;
			if (selected)
				ImGui::PopStyleColor();
		}

		if (ImGui::Button("Back"))
			index = 0;

		return false;
	}

	bool MI_Backtrader::DrawSubMenu(STBT& stbt, uint32_t& index)
	{
		if (running)
		{
			DrawLog(stbt);
			return false;
		}
		
		if (subIndex == btmiPortfolio)
			DrawPortfolioSubMenu(stbt);
		if (subIndex == btmiAlgorithms)
			DrawAlgorithmSubMenu(stbt);
		if (subIndex == btmiRunInfo)
			DrawRunSubMenu(stbt);
		return false;
	}

	bool MI_Backtrader::DrawFree(STBT& stbt, uint32_t& index)
	{
		if(!running)
			MI_Symbols::TryRenderSymbol(stbt, timeSeries, names, enabled, symbolIndex, normalizeGraph);
		BackTest(stbt, true);
		return false;
	}

	const char* MI_Backtrader::GetMenuTitle()
	{
		return "Backtrader";
	}

	const char* MI_Backtrader::GetSubMenuTitle()
	{
		return "Details";
	}

	const char* MI_Backtrader::GetDescription()
	{
		return "Test trade algorithms \nin paper trading.";
	}

	void MI_Backtrader::Unload(STBT& stbt)
	{
	}

	void MI_Backtrader::BackTest(STBT& stbt, bool render)
	{
		if (running)
		{
			const auto runInfo = GetRunInfo(stbt);
			if (!runInfo.valid)
			{
				stbt.output.Add() = "ERROR: Buffer is larger than run length. \n Aborting run.";
				running = false;
				stbt.arena.DestroyScope(runningScope);
			}

			auto& bot = stbt.bots[algoIndex];

			bool canFinish = !pauseOnFinish;
			if (runIndex >= runInfo.length - 1)
				canFinish = canFinish ? !pauseOnFinishAll : false;
			bool canEnd = false;

			if(render)
				RenderRun(stbt, runInfo, canFinish, canEnd);

			if (runIndex == -1)
			{
				runIndex = 0;
				runDayIndex = -1;
				runTimePoint = std::chrono::system_clock::now();
			}
			else if (runIndex >= runInfo.length)
			{
				running = false;
			}
			else
			{
				// If this is a new run, set everything up.
				if (runDayIndex == -1)
				{
					// If random, decide on day.
					const int32_t buffer = std::atoi(buffBuffer);
					const auto tCurrent = runInfo.to;
					const auto cdiff = difftime(tCurrent, runInfo.from);
					const uint32_t cdaysDiff = cdiff / 60 / 60 / 24;
					const uint32_t maxDiff = runInfo.daysDiff - buffer - runInfo.runLength;

					if (randomizeDate)
					{
						const uint32_t randOffset = rand() % maxDiff;
						runOffset = cdaysDiff - randOffset;
					}
					else
						runOffset = cdaysDiff - buffer;

					const auto tS = GetTime(0);
					const auto sdiff = difftime(tS, runInfo.from);
					const uint32_t sdaysDiff = sdiff / 60 / 60 / 24;
					runOffset += sdaysDiff;

					// Fill portfolio.
					portfolio.liquidity = std::atof(buffers[0]);
					for (uint32_t i = 0; i < timeSeries.length; i++)
						portfolio.stocks[i].count = static_cast<uint32_t>(*buffers[i + 1]);

					stbtScope = STBTScope::Create(&portfolio, timeSeries);
					for (uint32_t i = 0; i < timeSeries.length; i++)
						trades[i].change = 0;

					runDayIndex = 0;
					if (bot.init)
						if (!bot.init(stbtScope, bot.userPtr, runOffset, runOffset - runInfo.runLength, 
							runIndex, runInfo.length, buffer, stbt.output))
							runDayIndex = runInfo.runLength;

					runScope = stbt.arena.CreateScope();
					runLog = Log::Create(stbt.arena, stbtScope, runOffset - runInfo.runLength, runOffset);
					stepCompleted = false;
					tpStart = std::chrono::steady_clock::now();

					portPoints = CreateArray<jv::gr::GraphPoint>(stbt.tempArena, runInfo.runLength);
					relPoints = CreateArray<jv::gr::GraphPoint>(stbt.tempArena, runInfo.runLength);
					pctPoints = CreateArray<jv::gr::GraphPoint>(stbt.tempArena, runInfo.runLength);
				}
				// If this run is completed, either start a new run or quit.
				if (runDayIndex == runInfo.runLength)
				{
					if (canFinish || canEnd)
					{
						// Save the average of all runs.
						for (uint32_t i = 0; i < runInfo.runLength; i++)
						{
							auto& p = genPoints[i];
							p.close *= runIndex;
							p.close += relPoints[i].close;
							p.close /= runIndex + 1;

							p.open = p.close;
							p.high = p.close;
							p.low = p.close;
						}

						if (bot.cleanup)
							bot.cleanup(stbtScope, bot.userPtr, stbt.output);
						runDayIndex = -1;
						runIndex++;

						stbt.arena.DestroyScope(runScope);

						if (canEnd)
							runIndex = runInfo.length;

						if (log)
						{
							std::string s = std::format("Logs/{:%d-%m-%Y %H-%M-%OS}-", runTimePoint);
							s += std::to_string(runIndex) + ".log";
							Log::Save(runLog, s.c_str());
						}	
					}
				}
				else if(!stepwise || !stepCompleted)
				{
					auto tpEnd = std::chrono::steady_clock::now();
					auto diff = std::chrono::duration_cast<std::chrono::microseconds>(tpEnd - tpStart).count();
					timeElapsed += diff;
					tpStart = tpEnd;

					const uint32_t dayOffsetIndex = runOffset - runDayIndex;
					const float fee = std::atof(feeBuffer);
					const auto& stocks = portfolio.stocks;

					// Execute trades.
					for (uint32_t i = 0; i < timeSeries.length; i++)
					{
						const auto open = timeSeries[i].open[dayOffsetIndex];
						auto& trade = trades[i];
						auto& stock = portfolio.stocks[i];
						const float feeMod = (1.f + fee * (trade.change > 0 ? 1 : -1));

						// Limit max buys.
						if (trade.change > 0)
						{
							const uint32_t maxBuys = floor(portfolio.liquidity / (open * feeMod));
							trade.change = Min<int32_t>(maxBuys, trade.change);
						}
						// Limit max sells.
						if (trade.change < 0)
						{
							const int32_t maxSells = stock.count;
							trade.change = Max<int32_t>(-maxSells, trade.change);
						}

						float change = trade.change * open;
						change *= feeMod;

						stock.count += trade.change;
						portfolio.liquidity -= change;
						trade.change = 0;
					}

					float portfolioValue = 0;
					for (uint32_t i = 0; i < timeSeries.length; i++)
					{
						auto& num = runLog.numsInPort[i][runDayIndex] = stocks[i].count;
						const auto close = timeSeries[i].close[dayOffsetIndex];
						portfolioValue += close * num;
						runLog.stockCloses[i][runDayIndex] = close;
					}

					runLog.portValues[runDayIndex] = portfolioValue;
					runLog.liquidities[runDayIndex] = portfolio.liquidity;

					float close = 0;
					float closeStart = 0;

					for (uint32_t j = 0; j < timeSeries.length; j++)
					{
						const auto& series = timeSeries[j];
						close += series.close[runOffset - runDayIndex];
						closeStart += series.close[runOffset];
					}

					const float pct = close / closeStart;
					const float rel = (portfolioValue + portfolio.liquidity) / 
						(runLog.portValues[0] + runLog.liquidities[0]) / pct;

					runLog.marktPct[runDayIndex] = pct;
					runLog.marktRel[runDayIndex] = rel;

					if (!bot.update(stbtScope, trades, dayOffsetIndex, bot.userPtr, stbt.output))
						runDayIndex = runInfo.runLength;
					else
						runDayIndex++;
					stepCompleted = true;
				}
			}

			RenderGraphs(stbt, runInfo, render);
			if (instantMode && running && runDayIndex != runInfo.runLength)
				BackTest(stbt, false);
		}
	}
	void MI_Backtrader::DrawLog(STBT& stbt)
	{
		if (runDayIndex == -1)
			return;

		const uint32_t drawCap = 50;
		const uint32_t length = Min(runDayIndex, drawCap);
		const uint32_t start = drawCap > runDayIndex ? 0 : runDayIndex - drawCap;

		for (uint32_t i = 0; i < length; i++)
		{	
			std::string dayText = "---DAY ";
			dayText += std::to_string(runDayIndex - length + i + 1);
			dayText += "---";
			ImGui::Text(dayText.c_str());

			const float col = .6f;
			ImGui::PushStyleColor(ImGuiCol_Text, { col, col, col, 1 });

			const float portV = runLog.portValues[start + i];
			const float liqV = runLog.liquidities[start + i];

			std::string totalText = "Total Value: ";
			totalText += std::to_string((int)(portV + liqV));
			ImGui::Text(totalText.c_str());

			std::string portText = "Port Value: ";
			portText += std::to_string((int)portV);
			ImGui::Text(portText.c_str());

			std::string liquidText = "Liquidity: ";
			liquidText += std::to_string((int)liqV);
			ImGui::Text(liquidText.c_str());

			const int32_t pId = start + i - 1;
			if (pId >= 0)
			{
				const float portVp = runLog.portValues[pId];
				const float liqVp = runLog.liquidities[pId];

				const auto change = (int)((portV + liqV) - (portVp + liqVp));
				if (change != 0)
				{
					ImVec4 tradeCol = change > 0 ? ImVec4{ 0, 1, 0, 1 } : ImVec4{ 1, 0, 0, 1 };
					ImGui::PushStyleColor(ImGuiCol_Text, tradeCol);
					std::string changeText = "Change: ";
					changeText += std::to_string(change);
					ImGui::Text(changeText.c_str());
					ImGui::PopStyleColor();
				}
			}

			// Print trades.
			if (i > 0)
			{
				for (uint32_t j = 0; j < timeSeries.length; j++)
				{
					const auto& curLog = runLog.numsInPort[j];
					const int diff = curLog[runDayIndex - length + i - 1] - curLog[runDayIndex - length + i];
					if (diff == 0)
						continue;

					std::string tradeText = diff < 0 ? "[BUY] " : "[SELL] ";
					tradeText += portfolio.stocks[j].symbol;
					tradeText += " x";
					tradeText += std::to_string(diff * (diff < 0 ? -1 : 1));
					ImGui::Text(tradeText.c_str());
				}
			}

			ImGui::PopStyleColor();
		}
	}
	RunInfo MI_Backtrader::GetRunInfo(STBT& stbt)
	{
		RunInfo info{};

		info.from = mktime(&stbt.from);
		info.to = mktime(&stbt.to);

		const auto diff = difftime(info.to, info.from);
		info.daysDiff = diff / 60 / 60 / 24;

		info.length = std::atoi(runCountBuffer);
		info.runLength = randomizeDate ? std::atoi(lengthBuffer) : info.daysDiff;
		info.buffer = std::atoi(buffBuffer);
		info.valid = info.buffer < info.runLength;
		info.runLength -= info.buffer;
		return info;
	}

	void MI_Backtrader::DrawPortfolioSubMenu(STBT& stbt)
	{
		ImGui::Text("Portfolio");
		TryDrawTutorialText(stbt, "Starting cash.");

		uint32_t index = 0;
		ImGui::InputText("Cash", buffers[0], 9, ImGuiInputTextFlags_CharsScientific);

		TryDrawTutorialText(stbt, "Number of starting stocks\nin portfolio per symbol.");

		for (uint32_t i = 0; i < names.length; i++)
		{
			if (!enabled[i])
				continue;
			++index;

			ImGui::PushID(i);
			if (ImGui::InputText("##", buffers[index], 9, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(buffers[index]);
				n = Max(n, 0);
				snprintf(buffers[index], sizeof(buffers[index]), "%i", n);
			}
			ImGui::PopID();
			ImGui::SameLine();

			const bool selected = symbolIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

			const auto symbol = names[i].c_str();
			if (ImGui::Button(symbol))
				symbolIndex = i;

			if (selected)
				ImGui::PopStyleColor();
		}
	}

	void MI_Backtrader::DrawAlgorithmSubMenu(STBT& stbt)
	{
		for (uint32_t i = 0; i < stbt.bots.length; i++)
		{
			auto& bot = stbt.bots[i];
			const bool selected = i == algoIndex;

			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });
			if (ImGui::Button(bot.name))
				algoIndex = i;
			if (selected)
				ImGui::PopStyleColor();

			if (selected)
			{
				ImGui::Text(bot.author);
				ImGui::Text(bot.description);

				for (uint32_t i = 0; i < bot.boolsLength; i++)
					ImGui::Checkbox(bot.boolsNames[i], &bot.bools[i]);

				for (uint32_t i = 0; i < bot.buffersLength; i++)
				{
					ImGui::Text(bot.buffersNames[i]);
					ImGui::InputText("##", bot.buffers[i], bot.bufferSizes[i]);
				}
			}
		}
	}

	void MI_Backtrader::DrawRunSubMenu(STBT& stbt)
	{
		TryDrawTutorialText(stbt, "Amount of runs.");
		if (ImGui::InputText("Runs", runCountBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
		{
			int32_t n = std::atoi(runCountBuffer);
			n = Max(n, 1);
			snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);
		}

		TryDrawTutorialText(stbt, "Difference between starting\ndate and minimum run start\ndate.");
		if (ImGui::InputText("Buffer", buffBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
		{
			int32_t n = std::atoi(buffBuffer);
			n = Max(n, 1);
			snprintf(buffBuffer, sizeof(buffBuffer), "%i", n);
		}
		TryDrawTutorialText(stbt, "Fee on buying or\nselling stocks.\n1 = 100%");
		if (ImGui::InputText("Fee", feeBuffer, 8, ImGuiInputTextFlags_CharsDecimal))
		{
			float n = std::atof(feeBuffer);
			n = Max(n, 0.f);
			snprintf(feeBuffer, sizeof(feeBuffer), "%f", n);
		}

		TryDrawTutorialText(stbt, "Number of days in\nzoomed in view of\nportfolio and market\naverage.");
		if (ImGui::InputText("Zoom", zoomBuffer, 3, ImGuiInputTextFlags_CharsDecimal))
		{
			int32_t n = std::atoi(zoomBuffer);
			n = Clamp(n, 2, MAX_ZOOM);
			snprintf(zoomBuffer, sizeof(zoomBuffer), "%i", n);
		}
		TryDrawTutorialText(stbt, "If true, will skip visualization.");
		ImGui::Checkbox("Instant Mode", &instantMode);
		TryDrawTutorialText(stbt, "If enabled, pauses every\nX days, where X = Batches.");
		ImGui::Checkbox("Stepwise", &stepwise);
		TryDrawTutorialText(stbt, "Pause after run.");
		ImGui::Checkbox("Pause On Finish", &pauseOnFinish);
		TryDrawTutorialText(stbt, "Pause after final run.");
		ImGui::Checkbox("Pause On Finish ALL", &pauseOnFinishAll);
		TryDrawTutorialText(stbt, "Randomizes date (within\nthe given start/end\ndates).");
		ImGui::Checkbox("Randomize Date", &randomizeDate);
		TryDrawTutorialText(stbt, "Save results of the run\nto a text file after\nit's finished.");
		ImGui::Checkbox("Log", &log);

		if (randomizeDate)
		{
			TryDrawTutorialText(stbt, "Length of the\nrandomized run.");
			if (ImGui::InputText("Length", lengthBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(lengthBuffer);
				n = Max(n, 0);
				snprintf(lengthBuffer, sizeof(lengthBuffer), "%i", n);
			}
		}

		TryDrawTutorialText(stbt, "Begin the run(s).");
		if (ImGui::Button("Run"))
		{
			bool valid = true;
			if (algoIndex == -1)
			{
				valid = false;
				stbt.output.Add() = "ERROR: No algorithm selected!";
			}

			if (symbolIndex == -1)
			{
				valid = false;
				stbt.output.Add() = "ERROR: No symbols available!";
			}

			if (valid)
			{
				// check buffer w/ min date
				const int32_t buffer = std::atoi(buffBuffer);
				const int32_t length = std::atoi(lengthBuffer);

				const auto tFrom = mktime(&stbt.from);
				const auto tTo = mktime(&stbt.to);

				const auto diff = difftime(tTo, tFrom);
				const uint32_t daysDiff = diff / 60 / 60 / 24;

				if (daysDiff < buffer + 1)
				{
					valid = false;
					stbt.output.Add() = "ERROR: Buffer range is out of scope!";
				}

				if (randomizeDate && daysDiff < buffer + length)
				{
					valid = false;
					stbt.output.Add() = "ERROR: Length is out of scope!";
				}

				if (valid)
				{
					running = true;
					runIndex = -1;
					timeElapsed = 0;
					runningScope = stbt.arena.CreateScope();

					const auto runInfo = GetRunInfo(stbt);
					genPoints = CreateArray<jv::gr::GraphPoint>(stbt.tempArena, runInfo.runLength);
				}
			}
		}
	}
	void MI_Backtrader::RenderRun(STBT& stbt, const RunInfo& runInfo, bool& canFinish, bool& canEnd)
	{
		MI_Symbols::DrawBottomRightWindow("Current Run");

		const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
		const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

		std::string runText = "Epoch " + std::to_string(runIndex + 1);
		runText += "/";
		runText += std::to_string(runInfo.length);

		if (runDayIndex != -1)
		{
			runText += " Day " + std::to_string(runDayIndex);
			runText += "/";
			runText += std::to_string(runInfo.runLength);
		}
		if (runIndex == -1)
			runText = "Preprocessing data.";
		ImGui::Text(runText.c_str());

		if (!stepwise)
		{
			TryDrawTutorialText(stbt, "Time Elapsed/Remaining.");
			std::string elapsed = "Elapsed/Remaining: " + ConvertSecondsToHHMMSS(timeElapsed / 1e6) + "/";

			float e = timeElapsed;
			e /= runDayIndex + runIndex * runInfo.runLength;
			const float avrFrame = e;
			const float totalDuration = avrFrame * (runInfo.length * runInfo.runLength);
			e *= (runInfo.length - runIndex - 1) * runInfo.runLength + (runInfo.runLength - runDayIndex);
			elapsed += ConvertSecondsToHHMMSS(e / 1e6);
			ImGui::Text(elapsed.c_str());
			//ImGui::Text(ConvertSecondsToHHMMSS(totalDuration / 1e6).c_str());
		}

		if (runDayIndex >= runInfo.runLength && (pauseOnFinish || pauseOnFinishAll))
		{
			TryDrawTutorialText(stbt, "[CONTINUE]: End current run.");
			TryDrawTutorialText(stbt, "[BREAK]: Abort all queued runs.");
			if (ImGui::Button("Continue"))
				canFinish = true;
			ImGui::SameLine();
			if (runIndex < runInfo.length - 1 && ImGui::Button("Break"))
				canEnd = true;
		}
		else if (stepwise && stepCompleted)
		{
			TryDrawTutorialText(stbt, "[CONTINUE]: Continue current run.");
			TryDrawTutorialText(stbt, "[BREAK]: Abort all queued runs.");
			if (ImGui::Button("Continue"))
				stepCompleted = false;
			ImGui::SameLine();
			if (ImGui::Button("Break"))
			{
				runDayIndex = runInfo.runLength;
				stepCompleted = false;
				canFinish = true;
				canEnd = true;
			}
			ImGui::SameLine();
			ImGui::PushItemWidth(40);
			if (ImGui::InputText("Zoom", zoomBuffer, 3, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(zoomBuffer);
				n = Clamp(n, 2, MAX_ZOOM);
				snprintf(zoomBuffer, sizeof(zoomBuffer), "%i", n);
			}

			if (ImGui::Button("Stepwise off"))
				stepwise = false;
		}
		else
		{
			if (ImGui::Button("Stepwise on"))
				stepwise = true;
		}

		ImGui::End();

		MI_Symbols::DrawTopRightWindow("Stocks", true, true);
		TryDrawTutorialText(stbt, "[SYMBOl][AMOUNT][VALUE][CHANGE].");
		TryDrawTutorialText(stbt, "[REL]: Portfolio relative to stock market.");
		TryDrawTutorialText(stbt, "[MARK]: Market average.");

		std::string liquidity = "Liquidity: ";
		uint32_t ILiq = round(portfolio.liquidity);
		liquidity += std::to_string(ILiq);
		ImGui::Text(liquidity.c_str());

		const uint32_t dayOffsetIndex = runOffset - runDayIndex;
		std::string portValue = "Port Value: ";
		float v = portfolio.liquidity;

		for (uint32_t i = 0; i < timeSeries.length; i++)
		{
			const auto& stock = portfolio.stocks[i];
			const float val = stock.count * timeSeries[i].close[dayOffsetIndex];
			v += val;

			std::string t = stock.symbol;
			t += ": ";
			t += std::to_string(stock.count);
			t += ", ";
			t += std::to_string(int(round(val)));
			t += " ";

			const int32_t change = trades[i].change;
			if (change != 0)
			{
				ImVec4 col = change > 0 ? ImVec4{ 0, 1, 0, 1 } : ImVec4{ 1, 0, 0, 1 };
				ImGui::PushStyleColor(ImGuiCol_Text, col);

				if (change > 0)
					t += "+";
				t += std::to_string(change);
			}

			ImGui::Text(t.c_str());

			if (change != 0)
				ImGui::PopStyleColor();
		}
		uint32_t iV = round(v);

		portValue += std::to_string(iV);
		ImGui::Text(portValue.c_str());

		ImGui::End();
	}

	void MI_Backtrader::RenderGraphs(STBT& stbt, const RunInfo& runInfo, const bool render)
	{
		// Draw the graphs. Only possible if there are at least 2 graph points.
		if (runDayIndex > 0 && runDayIndex != -1)
		{
			const uint32_t dayOffsetIndex = runOffset - runDayIndex;
			const uint32_t l = runDayIndex >= runInfo.runLength ? runInfo.runLength - 1 : runDayIndex;
			const auto tScope = stbt.tempArena.CreateScope();

			const uint32_t i = l - 1;
			const auto v = runLog.portValues[i] + runLog.liquidities[i];
			const float rel = runLog.marktRel[i];
			const float pct = runLog.marktPct[i];

			portPoints[i].open = v;
			portPoints[i].close = v;
			portPoints[i].high = v;
			portPoints[i].low = v;

			relPoints[i].open = rel;
			relPoints[i].close = rel;
			relPoints[i].high = rel;
			relPoints[i].low = rel;

			pctPoints[i].open = pct;
			pctPoints[i].close = pct;
			pctPoints[i].high = pct;
			pctPoints[i].low = pct;

			if (!render)
			{
				stbt.tempArena.DestroyScope(tScope);
				return;
			}

			auto colors = LoadRandColors(stbt.tempArena, 5);
			const float ratio = stbt.renderer.GetAspectRatio();

			glm::vec2 grPos = { 0, 0 };
			grPos.x += .36f;
			grPos.y += .02f;

			jv::gr::DrawGraphInfo drawInfos[3]{};

			jv::gr::DrawGraphInfo drawInfo{};
			drawInfo.aspectRatio = ratio;
			drawInfo.position = grPos;
			drawInfo.scale = glm::vec2(.9);
			drawInfo.points = portPoints.ptr;
			drawInfo.length = l;
			drawInfo.textIsButton = true;
			drawInfo.color = colors[0];
			drawInfo.title = "port";
			drawInfos[0] = drawInfo;

			const float top = .8;
			const float bot = -.265;
			const float center = (top + bot) / 2;

			drawInfo.position = { .85f, center };
			drawInfo.scale = glm::vec2(1) / 3.f;
			drawInfo.points = pctPoints.ptr;
			drawInfo.color = colors[1];
			drawInfo.title = "mark";
			drawInfos[1] = drawInfo;

			drawInfo.position.y = bot;
			drawInfo.points = relPoints.ptr;
			drawInfo.color = colors[2];
			drawInfo.title = "rel";
			drawInfos[2] = drawInfo;

			// Switch between graphs if selected and give the full name to the big graph.
			const char* fullTitles[3]
			{
				"portfolio value",
				"market average",
				"relative to market"
			};

			drawInfos[highlightedGraphIndex].title = fullTitles[highlightedGraphIndex];
			if (highlightedGraphIndex != 0)
			{
				auto t = drawInfos[0];
				auto& a = drawInfos[0];
				auto& b = drawInfos[highlightedGraphIndex];

				a.points = b.points;
				a.title = b.title;
				b.points = t.points;
				b.title = t.title;
			}

			// If relative gains are debugged.
			if (highlightedGraphIndex == 2 && runIndex > 0)
			{
				auto genInfo = drawInfos[0];
				genInfo.title = nullptr;
				genInfo.points = genPoints.ptr;
				genInfo.color = glm::vec4(0, 1, 0, 1);
				stbt.renderer.DrawGraph(genInfo);
			}

			stbt.renderer.SetLineWidth(2);
			if (stbt.renderer.DrawGraph(drawInfos[0]))
				highlightedGraphIndex = 0;
			stbt.renderer.SetLineWidth(1);

			for (uint32_t i = 0; i < 2; i++)
				if (stbt.renderer.DrawGraph(drawInfos[i + 1]))
					highlightedGraphIndex = highlightedGraphIndex == i + 1 ? 0 : i + 1;

			const uint32_t zoom = std::stoi(zoomBuffer);

			if (l >= zoom && zoom > 0 && zoom <= Min((uint32_t)MAX_ZOOM, runInfo.runLength))
			{
				drawInfo.noBackground = false;
				std::string zoomPort = "port" + std::to_string(zoom);

				drawInfo.position.y = top;
				drawInfo.points = &portPoints.ptr[l - zoom];
				drawInfo.length = zoom;
				drawInfo.color = colors[3];
				drawInfo.title = zoomPort.c_str();
				stbt.renderer.DrawGraph(drawInfo);

				std::string zoomMarket = "mark" + std::to_string(zoom);
				drawInfo.position.x -= .3f;
				drawInfo.position.y = .8f;
				drawInfo.points = &pctPoints.ptr[l - zoom];
				drawInfo.color = colors[4];
				drawInfo.title = zoomMarket.c_str();
				stbt.renderer.DrawGraph(drawInfo);
			}

			stbt.tempArena.DestroyScope(tScope);
		}
	}
}