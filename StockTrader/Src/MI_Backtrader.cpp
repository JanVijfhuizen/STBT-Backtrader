#include "pch.h"
#include "MenuItems/MI_Backtrader.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>
#include <Ext/ImGuiLoadBar.h>
#include <Utils/UT_Time.h>

namespace jv::bt
{
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

		uint32_t index = 0;
		for (size_t i = 0; i < enabled.length; i++)
		{
			if (!enabled[i])
				continue;
			uint32_t _;
			timeSeries[index++] = MI_Symbols::LoadSymbol(stbt, i, names, _);
		}

		auto namesCharPtrs = CreateArray<const char*>(stbt.tempArena, names.length);
		for (uint32_t i = 0; i < names.length; i++)
			namesCharPtrs[i] = names[i].c_str();

		portfolio = Portfolio::Create(stbt.arena, namesCharPtrs.ptr, names.length);
		for (uint32_t i = 0; i < names.length; i++)
			portfolio.stocks[i].symbol = names[i].c_str();

		subIndex = 0;
		symbolIndex = -1;
		if (enabled.length > 0)
			symbolIndex = 0;
		normalizeGraph = false;

		algoIndex = -1;
		stepwise = false;
		log = false;
		pauseOnFinish = false;
		running = false;

		trades = stbt.arena.New<STBTTrade>(timeSeries.length);

		uint32_t n = 1;
		snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);
		snprintf(batchBuffer, sizeof(batchBuffer), "%i", n);
		n = 0;
		snprintf(buffBuffer, sizeof(buffBuffer), "%i", n);
		float f = 1e-3f;
		snprintf(feeBuffer, sizeof(feeBuffer), "%f", f);
		f = 10000;
		snprintf(buffers[0], sizeof(buffers[0]), "%f", f);

		stbt.tempArena.DestroyScope(tempScope);
		subScope = stbt.arena.CreateScope();
	}

	bool MI_Backtrader::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		const char* buttons[]
		{
			"Environment",
			"Algorithms",
			"Run Info"
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
		if (subIndex == btmiPortfolio)
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
		if (subIndex == btmiAlgorithms)
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
				}
			}
		}
		if (subIndex == btmiRunInfo)
		{
			if (ImGui::InputText("Buffer", buffBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(buffBuffer);
				n = Max(n, 0);
				snprintf(buffBuffer, sizeof(buffBuffer), "%i", n);
			}

			if (ImGui::InputText("Fee", feeBuffer, 8, ImGuiInputTextFlags_CharsDecimal))
			{
				float n = std::atof(feeBuffer);
				n = Max(n, 0.f);
				snprintf(feeBuffer, sizeof(feeBuffer), "%f", n);
			}

			if (ImGui::InputText("Runs", runCountBuffer, 4, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(runCountBuffer);
				n = Max(n, 1);
				snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);
			}

			if (ImGui::InputText("Batches", batchBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(batchBuffer);
				n = Max(n, 1);
				snprintf(batchBuffer, sizeof(batchBuffer), "%i", n);
			}

			ImGui::Checkbox("Stepwise", &stepwise);
			ImGui::Checkbox("Pause On Finish", &pauseOnFinish);
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
						batchId = 0;
					}
				}
			}
		}
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
			const auto tFrom = mktime(&stbt.from);
			const auto tTo = mktime(&stbt.to);

			const auto diff = difftime(tTo, tFrom);
			const uint32_t daysDiff = diff / 60 / 60 / 24;

			const int32_t length = std::atoi(runCountBuffer);
			const uint32_t runLength = randomizeDate ? std::atoi(lengthBuffer) : daysDiff;
			auto& bot = stbt.bots[algoIndex];

			bool canFinish = !pauseOnFinish;
			bool canEnd = false;

			if(render)
			{
				MI_Symbols::DrawBottomRightWindow("Current Run", true);

				const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
				const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

				std::string runText = "Epoch " + std::to_string(runIndex + 1);
				runText += "/";
				runText += std::to_string(length);

				if (runDayIndex != -1)
				{
					runText += " Day " + std::to_string(runDayIndex);
					runText += "/";
					runText += std::to_string(runLength);
				}
				if (runIndex == -1)
					runText = "Preprocessing data.";
				ImGui::Text(runText.c_str());
				
				if (runDayIndex >= runLength && pauseOnFinish)
				{
					if (ImGui::Button("Continue"))
						canFinish = true;
					ImGui::SameLine();
					if (ImGui::Button("Break"))
						canEnd = true;
				}
				else if (stepwise && stepCompleted)
				{
					if (ImGui::Button("Continue"))
						stepCompleted = false;
					ImGui::SameLine();
					if (ImGui::Button("Break"))
					{
						runDayIndex = runLength;
						stepCompleted = false;
						canFinish = true;
						canEnd = true;
					}
				}

				ImGui::End();

				MI_Symbols::DrawTopRightWindow("Stocks", true, true);

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
					ImGui::Text(t.c_str());
				}
				uint32_t iV = round(v);

				portValue += std::to_string(iV);
				ImGui::Text(portValue.c_str());

				ImGui::End();
			}

			if (runIndex == -1)
			{
				runIndex = 0;
				runDayIndex = -1;
			}
			else if (runIndex >= length)
			{
				running = false;
			}
			else
			{
				if (runDayIndex == -1)
				{
					// If random, decide on day.
					const int32_t buffer = std::atoi(buffBuffer);
					const auto tCurrent = GetTime(0);
					const auto cdiff = difftime(tCurrent, tFrom);
					const uint32_t cdaysDiff = cdiff / 60 / 60 / 24;
					const uint32_t maxDiff = daysDiff - buffer - runLength;

					if (randomizeDate)
					{
						const uint32_t randOffset = rand() % maxDiff;
						runOffset = cdaysDiff - randOffset;
					}
					else
						runOffset = cdaysDiff;

					// Fill portfolio.
					portfolio.liquidity = std::atof(buffers[0]);
					for (uint32_t i = 0; i < timeSeries.length; i++)
						portfolio.stocks[i].count = static_cast<uint32_t>(*buffers[i + 1]);

					stbtScope = STBTScope::Create(&portfolio, timeSeries);
					for (uint32_t i = 0; i < timeSeries.length; i++)
						trades[i].change = 0;

					if (bot.init)
						bot.init(stbtScope, bot.userPtr);
					runDayIndex = 0;

					runScope = stbt.arena.CreateScope();
					runLog = Log::Create(stbt.arena, stbtScope, runOffset - runLength, runOffset);
					stepCompleted = false;
				}
				if (runDayIndex == runLength)
				{
					if (canFinish || canEnd)
					{
						if (bot.cleanup)
							bot.cleanup(stbtScope, bot.userPtr);
						runDayIndex = -1;
						runIndex++;

						stbt.arena.DestroyScope(runScope);

						if (canEnd)
							runIndex = length;
					}
				}
				else if(!stepwise || !stepCompleted)
				{
					const uint32_t dayOffsetIndex = runOffset - runDayIndex;
					const float fee = std::atof(feeBuffer);
					const auto& stocks = portfolio.stocks;

					float portfolioValue = 0;

					for (uint32_t i = 0; i < timeSeries.length; i++)
					{
						auto& num = runLog.numsInPort[i][runDayIndex] = stocks[i].count;
						portfolioValue += timeSeries[i].close[dayOffsetIndex] * num;
					}
					runLog.portValues[runDayIndex] = portfolioValue;
					runLog.liquidities[runDayIndex] = portfolio.liquidity;

					// Execute trades.
					for (uint32_t i = 0; i < timeSeries.length; i++)
					{
						auto& trade = trades[i];
						auto& stock = portfolio.stocks[i];
						float change = trade.change * timeSeries[i].open[dayOffsetIndex];
						const float feeMod = (1.f + fee * (change > 0 ? 1 : -1));
						change *= feeMod;

						const bool enoughInStock = trade.change > 0 ? true : trade.change <= stock.count;

						if (change < portfolio.liquidity && enoughInStock)
						{
							stock.count += trade.change;
							portfolio.liquidity -= change;
						}

						trade.change = 0;
					}

					bot.update(stbtScope, trades, dayOffsetIndex, bot.userPtr);
					runDayIndex++;
					stepCompleted = true;
				}
			}

			if (render)
			{
				if (runDayIndex > 0 && runDayIndex != -1)
				{
					const uint32_t l = runDayIndex >= runLength ? runLength - 1 : runDayIndex;
					const auto tScope = stbt.tempArena.CreateScope();
					auto graphPoints = CreateArray<jv::gr::GraphPoint>(stbt.tempArena, runLength);
					for (uint32_t i = 0; i < l; i++)
					{
						const auto v = runLog.portValues[i] + runLog.liquidities[i];
						graphPoints[i].open = v;
						graphPoints[i].close = v;
						graphPoints[i].high = v;
						graphPoints[i].low = v;
					}

					stbt.renderer.DrawGraph({ 0.5, 0 },
						glm::vec2(stbt.renderer.GetAspectRatio(), 1),
						graphPoints.ptr, l, gr::GraphType::line,
						false, true, glm::vec4(1), l);

					stbt.tempArena.DestroyScope(tScope);
				}
			}

			const int32_t batchLength = std::atoi(batchBuffer);
			if (++batchId < batchLength && running)
				BackTest(stbt, false);
			else
				batchId = 0;
		}
	}
}