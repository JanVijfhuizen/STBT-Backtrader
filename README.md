# STBT - Stock Trading Back Tester

This is a tool to test out stock trading algorithms on real historical data.
It is an ongoing project but it is in a functional state and can be used freely under the MIT license.

![image](https://github.com/user-attachments/assets/a3ed68d9-c35b-4d50-be92-c5b4bc221ac5)

# How does it work?

First, you need to download the individual symbol data, and then enable all symbols you want to use in backtesting.

![image](https://github.com/user-attachments/assets/35a118cf-2171-477f-8611-b245680694e4)

Then the application downloads all relevant information for the symbols and saves it locally.
Afterwards you can run your algorithms on historical data and get detailed feedback on how it performed.
This includes a scatter plot, performance bell curve, averages and more. 
You can also go through the simulation step by step if needed.
A run can also be logged to a text file for external use.

![image](https://github.com/user-attachments/assets/142d2629-fdd2-4135-91f0-322e45303d87)

The code can be set up rather quickly:

```
jv::bt::STBTBot bot;
bot.name = "GA trader";
bot.description = "Genetic Algorithm Trading.";
bot.author = "jannie";
bot.init = jv::GATraderInit;
bot.update = jv::GATraderUpdate;
bot.cleanup = jv::GATraderCleanup;
bot.bools = &training;
bot.boolsNames = &boolsNames;
bot.boolsLength = 1;
bot.userPtr = this;

auto stbt = jv::bt::CreateSTBT(&bot, 1);
while (!stbt.Update())
	continue;
```

Basically, you just define the bots that you want to potentially test out and that's it.
The code is written with performance in mind and can run hundreds of simulations a second,
so it should be a good way of quickly testing out algorithms before commiting them to live trading.

![image](https://github.com/user-attachments/assets/e57a17a7-a243-4d81-a14d-74cd53f8d72f)

I will add a ready to go .exe in the future, but for now you'll need to build from source if you want to use it.


