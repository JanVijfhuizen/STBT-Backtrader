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

	enum class RunType
	{
		normal,
		stepwise,
		instant
	};
	
	enum BTMenuIndex
	{
		btmiPortfolio,
		btmiAlgorithms,
		btmiRunInfo
	};

	enum ShowIndex
	{
		current,
		betaScatter,
		bellCurve
	};

	void RenderShowIndexDropDown(MI_Backtrader& bt)
	{
		const char* windowNames[3]
		{
			"Current Run",
			"Beta Scatter",
			"Bell Curve"
		};

		ImGui::Combo("Show", &bt.showIndex, windowNames, 3);
	}

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

		colors = LoadRandColors(stbt.arena, timeSeries.length);
		portfolio = Portfolio::Create(stbt.arena, namesCharPtrs.ptr, names.length);
		for (uint32_t i = 0; i < c; i++)
			portfolio.stocks[i].symbol = namesCharPtrs[i];

		subIndex = 0;
		symbolIndex = -1;
		if (enabled.length > 0)
			symbolIndex = 0;
		normalizeGraph = false;

		algoIndex = -1;
		log = false;
		pauseOnFinish = false;
		pauseOnFinishAll = true;
		running = false;
		runType = 0;
		showIndex = 0;

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
		if (!running)
		{
			SymbolsDataDrawInfo drawInfo{};
			drawInfo.timeSeries = timeSeries.ptr;
			drawInfo.names = names.ptr;
			drawInfo.enabled = enabled.ptr;
			drawInfo.colors = colors.ptr;
			drawInfo.length = timeSeries.length;
			drawInfo.symbolIndex = &symbolIndex;
			drawInfo.normalizeGraph = &normalizeGraph;
			MI_Symbols::RenderSymbolData(stbt, drawInfo);
		}
		
		BackTest(stbt, true);
		// Instead of going recursively, do this. Otherwise I'd get a stack overflow.
		if(runType == static_cast<int>(RunType::instant))
			while(running && runDayIndex != runInfo.length)
				BackTest(stbt, false);
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
			auto& bot = stbt.bots[algoIndex];

			bool canFinish = !pauseOnFinish;
			if (runIndex >= runInfo.totalRuns - 1)
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
			else if (runIndex >= runInfo.totalRuns)
			{
				running = false;
				runDayIndex = -1;
			}
			else
			{
				// If this is a new run, set everything up.
				if (runDayIndex == -1)
				{
					// If random, decide on day.
					if (randomizeDate)
					{
						const uint32_t maxDiff = runInfo.range - runInfo.buffer - runInfo.length;
						const uint32_t randOffset = rand() % maxDiff;
						runInfo.to = randOffset;
						runInfo.from = runInfo.to + runInfo.length;
					}

					// Fill portfolio.
					portfolio.liquidity = std::atof(buffers[0]);
					for (uint32_t i = 0; i < timeSeries.length; i++)
						portfolio.stocks[i].count = static_cast<uint32_t>(*buffers[i + 1]);

					stbtScope = STBTScope::Create(&portfolio, timeSeries);
					for (uint32_t i = 0; i < timeSeries.length; i++)
						trades[i].change = 0;

					runDayIndex = 0;
					if (bot.init)
						if (!bot.init(stbtScope, bot.userPtr, runInfo.from, runInfo.to, 
							runIndex, runInfo.length, runInfo.buffer, stbt.output))
							runDayIndex = runInfo.length;

					runScope = stbt.arena.CreateScope();
					runLog = Log::Create(stbt.arena, stbtScope, runInfo.from, runInfo.to);
					stepCompleted = false;
					tpStart = std::chrono::steady_clock::now();

					portPoints = CreateArray<jv::gr::GraphPoint>(stbt.arena, runInfo.length);
					relPoints = CreateArray<jv::gr::GraphPoint>(stbt.arena, runInfo.length);
					pctPoints = CreateArray<jv::gr::GraphPoint>(stbt.arena, runInfo.length);
				}
				// If this run is completed, either start a new run or quit.
				if (runDayIndex == runInfo.length)
				{
					if (canFinish || canEnd)
					{
						// Save the average of all runs.
						for (uint32_t i = 0; i < runInfo.length; i++)
						{
							auto& r = gRelPoints[i];
							r.close *= runIndex;
							r.close += relPoints[i].close;
							r.close /= runIndex + 1;

							r.open = r.close;
							r.high = r.close;
							r.low = r.close;

							auto& p = gPortPoints[i];
							p.close *= runIndex;
							p.close += portPoints[i].close;
							p.close /= runIndex + 1;

							p.open = p.close;
							p.high = p.close;
							p.low = p.close;
						}
						avrDeviations[runIndex] = relPoints[runInfo.length - 1].close;

						// Save market & portfolio profit percentage seperately			
						scatterBeta[runIndex].x = pctPoints[runInfo.length - 1].close - 1.f;
						scatterBeta[runIndex].y = portPoints[runInfo.length - 1].close / portPoints[0].close - 1.f;

						scatterBetaRel[runIndex].x = scatterBeta[runIndex].x;
						scatterBetaRel[runIndex].y = relPoints[runInfo.length - 1].close - 1.f;

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
				else if((runType != static_cast<int>(RunType::stepwise)) || !stepCompleted)
				{
					auto tpEnd = std::chrono::steady_clock::now();
					auto diff = std::chrono::duration_cast<std::chrono::microseconds>(tpEnd - tpStart).count();
					timeElapsed += diff;
					tpStart = tpEnd;

					const uint32_t dayOffsetIndex = runInfo.from - runDayIndex;
					const float fee = std::atof(feeBuffer);
					const auto& stocks = portfolio.stocks;

					// Execute trades called on yesterday.
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

					// Update portfolio BEFORE doing trades for the next day.
					float portfolioValue = 0;
					for (uint32_t i = 0; i < timeSeries.length; i++)
					{
						auto& num = runLog.numsInPort[i][runDayIndex] = stocks[i].count;
						const auto close = timeSeries[i].close[dayOffsetIndex];
						portfolioValue += close * num;
						runLog.stockCloses[i][runDayIndex] = close;
					}

					// Update remaining info also BEFORE trading.
					runLog.portValues[runDayIndex] = portfolioValue;
					runLog.liquidities[runDayIndex] = portfolio.liquidity;

					float close = 0;
					float closeStart = 0;

					for (uint32_t j = 0; j < timeSeries.length; j++)
					{
						const auto& series = timeSeries[j];
						close += series.close[runInfo.from - runDayIndex];
						closeStart += series.close[runInfo.from];
					}

					const float pct = close / closeStart;
					const float rel = (portfolioValue + portfolio.liquidity) /
						(runLog.portValues[0] + runLog.liquidities[0]) / pct;

					runLog.marktPct[runDayIndex] = pct;
					runLog.marktRel[runDayIndex] = rel;

					if (!bot.update(stbtScope, trades, dayOffsetIndex, bot.userPtr, stbt.output))
						runDayIndex = runInfo.length;
					else
						runDayIndex++;
					stepCompleted = true;
				}
			}

			RenderGraphs(stbt, runInfo, render && (showIndex == ShowIndex::current));
			switch (showIndex)
			{
			case ShowIndex::betaScatter:
				RenderScatter(stbt, runInfo, render);
				break;
			case ShowIndex::bellCurve:
				RenderBellCurve(stbt, runInfo, render);
				break;
			default:
				break;
			}
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

			std::string portText = "Port Value: ";
			portText += std::to_string((int)portV);
			ImGui::Text(portText.c_str());

			std::string totalText = "Total Value: ";
			totalText += std::to_string((int)(portV + liqV));
			ImGui::Text(totalText.c_str());

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

	void MI_Backtrader::DrawPortfolioSubMenu(STBT& stbt)
	{
		ImGui::Text("Portfolio");
		
		uint32_t index = 0;
		ImGui::InputText("Cash", buffers[0], 9, ImGuiInputTextFlags_CharsScientific);

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
		if (ImGui::InputText("Runs", runCountBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
		{
			int32_t n = std::atoi(runCountBuffer);
			n = Max(n, 1);
			snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);
		}

		if (ImGui::InputText("Buffer", buffBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
		{
			int32_t n = std::atoi(buffBuffer);
			n = Max(n, 1);
			snprintf(buffBuffer, sizeof(buffBuffer), "%i", n);
		}
		
		if (ImGui::InputText("Fee", feeBuffer, 8, ImGuiInputTextFlags_CharsDecimal))
		{
			float n = std::atof(feeBuffer);
			n = Max(n, 0.f);
			snprintf(feeBuffer, sizeof(feeBuffer), "%f", n);
		}

		if (ImGui::InputText("Zoom", zoomBuffer, 3, ImGuiInputTextFlags_CharsDecimal))
		{
			int32_t n = std::atoi(zoomBuffer);
			n = Clamp(n, 2, MAX_ZOOM);
			snprintf(zoomBuffer, sizeof(zoomBuffer), "%i", n);
		}

		const char* items[]{ "Default", "Stepwise", "Instant"};
		ImGui::Combo("Type", &runType, items, 3);
		RenderShowIndexDropDown(*this);

		ImGui::Checkbox("Pause On Finish", &pauseOnFinish);
		ImGui::Checkbox("Pause On Finish ALL", &pauseOnFinishAll);
		ImGui::Checkbox("Randomize Date", &randomizeDate);
		ImGui::Checkbox("Log", &log);

		if (randomizeDate)
		{
			if (ImGui::InputText("Length", lengthBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(lengthBuffer);
				n = Max(n, 0);
				snprintf(lengthBuffer, sizeof(lengthBuffer), "%i", n);
			}
		}

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

				if (stbt.range - 1 < buffer)
				{
					valid = false;
					stbt.output.Add() = "ERROR: Buffer range is out of scope!";
				}

				const uint32_t daysDiff = stbt.range - 1 - buffer;
				const int32_t length = std::atoi(lengthBuffer);
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

					runInfo.range = stbt.range;
					runInfo.length = randomizeDate ? length : stbt.range - buffer;
					runInfo.buffer = buffer;
					runInfo.totalRuns = std::stoi(runCountBuffer);
					
					runInfo.from = runInfo.length;
					runInfo.to = 0;
					
					runningScope = stbt.arena.CreateScope();
					gRelPoints = CreateArray<jv::gr::GraphPoint>(stbt.arena, runInfo.length + buffer);
					gPortPoints = CreateArray<jv::gr::GraphPoint>(stbt.arena, runInfo.length + buffer);
					avrDeviations = stbt.arena.New<float>(runInfo.totalRuns);
					scatterBeta = stbt.arena.New<glm::vec2>(runInfo.totalRuns);
					scatterBetaRel = stbt.arena.New<glm::vec2>(runInfo.totalRuns);
				}
			}
		}
	}
	void MI_Backtrader::RenderRun(STBT& stbt, const RunInfo& runInfo, bool& canFinish, bool& canEnd)
	{
		if (runDayIndex == -1 || runDayIndex == 0)
			return;

		MI_Symbols::DrawBottomRightWindow("Current Run");

		const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
		const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

		std::string runText = "Epoch " + std::to_string(runIndex + 1);
		runText += "/";
		runText += std::to_string(runInfo.totalRuns);

		if (runDayIndex != -1)
		{
			runText += " Day " + std::to_string(runDayIndex);
			runText += "/";
			runText += std::to_string(runInfo.length);
		}
		if (runIndex == -1)
			runText = "Preprocessing data.";
		ImGui::Text(runText.c_str());

		if (runType != static_cast<int>(RunType::stepwise))
		{
			/*
			std::string elapsed = "Elapsed/Remaining: " + ConvertSecondsToHHMMSS(timeElapsed / 1e6) + "/";

			float e = timeElapsed;
			e /= runDayIndex + runIndex * runInfo.length;
			const float avrFrame = e;
			const float totalDuration = avrFrame * (runInfo.totalRuns);
			e *= (runInfo.totalRuns - runIndex - 1) * runInfo.totalRuns + (runInfo.length - runDayIndex);
			elapsed += ConvertSecondsToHHMMSS(e / 1e6);
			ImGui::Text(elapsed.c_str());
			//ImGui::Text(ConvertSecondsToHHMMSS(totalDuration / 1e6).c_str());
			*/
		}

		if (runDayIndex >= runInfo.length && (pauseOnFinish || pauseOnFinishAll))
		{
			RenderShowIndexDropDown(*this);

			if (ImGui::Button("Continue"))
				canFinish = true;
			if (runIndex < runInfo.totalRuns - 1)
			{
				ImGui::SameLine();
				if (ImGui::Button("Break"))
				{
					runIndex = runInfo.totalRuns;
					canEnd = true;
				}
			}	
		}
		else if (runType == static_cast<int>(RunType::stepwise) && stepCompleted)
		{
			if (ImGui::Button("Continue"))
				stepCompleted = false;
			ImGui::SameLine();
			if (ImGui::Button("Break"))
			{
				runIndex = runInfo.totalRuns;
				runDayIndex = runInfo.length;
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
				runType = static_cast<int>(RunType::normal);
		}
		else
		{
			if (ImGui::Button("Stepwise on"))
				runType = static_cast<int>(RunType::stepwise);
		}

		ImGui::End();

		if (showIndex != static_cast<int>(ShowIndex::current))
			return;

		MI_Symbols::DrawTopRightWindow("Stocks", true, true);
		
		float fLiquidity = runLog.liquidities[runDayIndex - 1];
		std::string liquidity = "Liquidity: ";
		uint32_t ILiq = round(fLiquidity);
		liquidity += std::to_string(ILiq);
		ImGui::Text(liquidity.c_str());

		const uint32_t dayOffsetIndex = runInfo.from - runDayIndex + 1;
		std::string portValue = "Port Value: ";
		float v = 0;

		for (uint32_t i = 0; i < timeSeries.length; i++)
		{
			auto& portStock = runLog.numsInPort[i];
			const uint32_t stockCount = portStock[runDayIndex - 1];

			const auto& stock = portfolio.stocks[i];
			const float val = stockCount * timeSeries[i].close[dayOffsetIndex];
			const int32_t change = trades[i].change;

			v += val;

			if (stock.count == 0 && change <= 0)
				continue;

			std::string t = stock.symbol;
			t += ": ";
			t += std::to_string(stockCount);
			t += ", ";
			t += std::to_string(int(round(val)));
			t += " ";
			
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

		std::string totalValue = "Total Value: ";
		v += fLiquidity;
		uint32_t iT = round(v);
		totalValue += std::to_string(iT);
		ImGui::Text(totalValue.c_str());

		if (runIndex > 0)
		{
			{
				// Average portfolio performance.
				auto avr = gPortPoints[runInfo.length - 1].close / gPortPoints[0].close * 100 - 100;
				std::stringstream stream;
				stream << std::fixed << std::setprecision(2) << avr;
				std::string s = stream.str();
				std::string avrStr = "AVR: " + s + "%%";
				ImGui::Text(avrStr.c_str());
			}
			
			// Relative to market average.
			auto relToMarkAvr = gRelPoints[runInfo.length - 1].close / gRelPoints[0].close * 100 - 100;
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << relToMarkAvr;
			std::string s = stream.str();
			std::string relToMarkAvrStr = "Rel AVR: " + s + "%%";
			ImGui::Text(relToMarkAvrStr.c_str());

			// Relative to market deviation.
			float dev = 0;
			for (uint32_t i = 0; i < runIndex; i++)
				dev += pow(avrDeviations[i] - relToMarkAvr, 2);
			dev /= runIndex;

			std::stringstream streamDev;
			streamDev << std::fixed << std::setprecision(2) << dev;
			std::string sD = streamDev.str();
			std::string avrRelDev = "Rel DEV: " + sD;
			ImGui::Text(avrRelDev.c_str());
		}

		ImGui::End();
	}

	void MI_Backtrader::RenderGraphs(STBT& stbt, const RunInfo& runInfo, const bool render)
	{
		// Draw the graphs. Only possible if there are at least 2 graph points.
		if (runDayIndex == -1 || runDayIndex < 1)
			return;

		const uint32_t dayOffsetIndex = runInfo.from - runDayIndex;
		const uint32_t l = runDayIndex >= runInfo.length ? runInfo.length : runDayIndex;
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

		jv::gr::DrawLineGraphInfo drawInfos[3]{};

		jv::gr::DrawLineGraphInfo drawInfo{};
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

		// If gains are debugged.
		if (highlightedGraphIndex == 0 && runIndex > 0)
		{
			auto gPortInfo = drawInfos[0];
			gPortInfo.title = nullptr;
			gPortInfo.points = gPortPoints.ptr;
			gPortInfo.color = glm::vec4(0, 1, 0, 1);
			stbt.renderer.DrawLineGraph(gPortInfo);
		}

		// If relative gains are debugged.
		if (highlightedGraphIndex == 2 && runIndex > 0)
		{
			auto gRelInfo = drawInfos[0];
			gRelInfo.title = nullptr;
			gRelInfo.points = gRelPoints.ptr;
			gRelInfo.color = glm::vec4(0, 1, 0, 1);
			stbt.renderer.DrawLineGraph(gRelInfo);
		}

		stbt.renderer.SetLineWidth(2);
		if (stbt.renderer.DrawLineGraph(drawInfos[0]))
			highlightedGraphIndex = 0;
		stbt.renderer.SetLineWidth(1);

		for (uint32_t i = 0; i < 2; i++)
			if (stbt.renderer.DrawLineGraph(drawInfos[i + 1]))
				highlightedGraphIndex = highlightedGraphIndex == i + 1 ? 0 : i + 1;

		const uint32_t zoom = std::stoi(zoomBuffer);

		if (l >= zoom && zoom > 0 && zoom <= Min((uint32_t)MAX_ZOOM, runInfo.length))
		{
			drawInfo.noBackground = false;
			std::string zoomPort = "port" + std::to_string(zoom);

			drawInfo.position.y = top;
			drawInfo.points = &portPoints.ptr[l - zoom];
			drawInfo.length = zoom;
			drawInfo.color = colors[3];
			drawInfo.title = zoomPort.c_str();
			stbt.renderer.DrawLineGraph(drawInfo);

			std::string zoomMarket = "mark" + std::to_string(zoom);
			drawInfo.position.x -= .3f;
			drawInfo.position.y = .8f;
			drawInfo.points = &pctPoints.ptr[l - zoom];
			drawInfo.color = colors[4];
			drawInfo.title = zoomMarket.c_str();
			stbt.renderer.DrawLineGraph(drawInfo);
		}

		stbt.tempArena.DestroyScope(tScope);
	}

	void MI_Backtrader::RenderBellCurve(STBT& stbt, const RunInfo& runInfo, const bool render)
	{
		if (!render)
			return;

		const uint32_t CHUNKS = 25;
		
		glm::vec2* arrs[]
		{
			scatterBeta,
			scatterBetaRel
		};

		glm::vec4 colors[]
		{
			glm::vec4(1, 0, 0, 1),
			glm::vec4(0, 1, 0, 1)
		};

		// Get max width for both scatters.
		float width = 0;
		for (uint32_t i = 0; i < 2; i++)
			for (uint32_t j = 0; j < runIndex; j++)
				width = Max(width, abs(arrs[i][j].y));

		for (uint32_t i = 0; i < 2; i++)
		{
			uint32_t distribution[CHUNKS]{};
			auto arr = arrs[i];

			const float m = static_cast<float>(CHUNKS - 1) / 2;

			for (uint32_t i = 0; i < runIndex; i++)
			{
				const float f = arr[i].y / width;
				float fPos = m + f * m;
				uint32_t pos = round(fPos);
				++distribution[pos];
			}

			glm::vec2 points[CHUNKS];
			for (uint32_t i = 0; i < CHUNKS; i++)
			{
				points[i].y = static_cast<float>(distribution[i]) / runIndex;
				points[i].x = -.5f + (1.f / static_cast<float>(CHUNKS - 1) * i);
			}

			glm::vec2 grPos = { 0, 0 };
			grPos.x += .5f;
			grPos.y += .14f;

			std::string title = "Bell Curve [R] Pct [G] Rel [X] ";
			title += std::format("{:.2f}", width * 100);
			title += "%%";
			
			gr::DrawScatterGraphInfo info{};
			info.aspectRatio = stbt.renderer.GetAspectRatio();
			info.position = grPos;
			info.points = points;
			info.length = CHUNKS;
			info.title = i == 0 ? title.c_str() : nullptr;
			info.scale = glm::vec2(1.3);
			info.colors = &colors[i];
			stbt.renderer.DrawScatterGraph(info);
		}
	}

	void MI_Backtrader::RenderScatter(STBT& stbt, const RunInfo& runInfo, const bool render)
	{
		if (!render)
			return;

		const auto tempScope = stbt.tempArena.CreateScope();

		glm::vec2 grPos = { 0, 0 };
		grPos.x += .5f;
		grPos.y += .14f;

		gr::DrawScatterGraphInfo info{};
		info.aspectRatio = stbt.renderer.GetAspectRatio();
		info.position = grPos;
		info.points = scatterBeta;
		info.length = runIndex;
		info.title = "Beta: [X] Market, [Y] Algorithm";
		info.scale = glm::vec2(1.3);
		stbt.renderer.DrawScatterGraph(info);

		stbt.tempArena.DestroyScope(tempScope);
	}
}