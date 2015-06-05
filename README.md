## Blackbird Bitcoin Arbitrage

### Introduction
Blackbird Bitcoin Arbitrage is a trading system that does automatic long/short arbitrage between Bitcoin exchanges. The software is written in C++ and runs on Linux.

### How It Works
Bitcoin is a new and inefficient market. Several Bitcoin exchanges exist and the proposed bid/ask can be different from an exchange to another. The purpose of this software is to automatically exploit the short-term bid/ask difference between two given Bitcoin exchanges. 

Here is an example with real data. The software analyzes the bid/ask information from two Bitcoin exchanges, Bitfinex and Bitstamp, every five seconds. At some point the spread between Bitfinex and Bitstamp prices is higher than an `ENTRY` threshold (first vertical dark line): an arbitrage opportunity exists and the system will buy Bitsamp and short sell Bitfinex. Then about 4.5 hours later the spread decreases below an `EXIT` threshold (second vertical dark line) so the system exits the market by selling Bitstamp and buying Bitfinex back.

<p align="center">
<img src="https://cloud.githubusercontent.com/assets/11370278/6548498/64764de6-c5d1-11e4-855b-c2eebb3b782b.png"  width="75%" alt="Spread Example">
</p>


### Disclaimer

__USE THE SOFTWARE AT YOUR OWN RISK. YOU ARE RESPONSIBLE FOR YOUR OWN MONEY. PAST PERFORMANCE IS NOT NECESSARILY INDICATIVE OF FUTURE RESULTS. THE AUTHORS AND ALL AFFILIATES ASSUME NO RESPONSIBILITY FOR YOUR TRADING RESULTS.__


### Live Results

As of today the program gets a 2% monthly return in average. It generates 8 trades per month on average and doesn't have any single losing trade thanks to the arbitrage mechanism. The trades usually last between 20 minutes and 7 days. The trade lengths are very variable since the system _always_ wait for the spread to close. This can take minutes or days but that is the reason why it doesn't get any losing trades. There is a control parameter named `MaxLength` that will automatically close a trade after 60 days. This parameter can be adapted or removed if needed.

### Console Output Example

This is what Blackbird looks like when it is started:

```
Blackbird Bitcoin Arbitrage
Version 0.0.2
DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK.

[ Targets ]
   Spread to enter: 0.30%
   Spread to exit: -0.40%

[ Current balances ]
   Bitfinex:    1,304.13 USD    0.000000 BTC
   OKCoin:      1,317.13 USD    0.000236 BTC
   Bitstamp:    36.10 USD       0.000000 BTC
   Kraken:      0.00 USD        0.000000 BTC

[ Cash exposure ]
   TEST cash used
   Value: $20.00

[ 05/31/2015 12:31:00 ]
   Bitfinex:    231.30 / 231.38
   OKCoin:      231.66 / 231.69
   Bitstamp:    231.62 / 231.77
   Kraken:      230.96 / 232.15
   ----------------------------
   Bitfinex/Kraken:     -0.18% [target  1.20%, min -0.18%, max -0.18%]
   OKCoin/Bitfinex:     -0.17% [target  1.10%, min -0.17%, max -0.17%]
   OKCoin/Kraken:       -0.32% [target  1.20%, min -0.32%, max -0.32%]
   Bitstamp/Bitfinex:   -0.20% [target  1.20%, min -0.20%, max -0.20%]
   Bitstamp/Kraken:     -0.35% [target  1.30%, min -0.35%, max -0.35%]
   Kraken/Bitfinex:     -0.36% [target  1.20%, min -0.36%, max -0.36%]

[ 05/31/2015 12:31:05 ]
   Bitfinex:    231.24 / 231.38
   OKCoin:      231.66 / 231.69
   Bitstamp:    231.62 / 231.77
   Kraken:      230.96 / 232.15
   ----------------------------
   Bitfinex/Kraken:     -0.18% [target  1.20%, min -0.18%, max -0.18%]
   OKCoin/Bitfinex:     -0.19% [target  1.10%, min -0.19%, max -0.17%]
   OKCoin/Kraken:       -0.32% [target  1.20%, min -0.32%, max -0.32%]
   Bitstamp/Bitfinex:   -0.23% [target  1.20%, min -0.23%, max -0.20%]
   Bitstamp/Kraken:     -0.35% [target  1.30%, min -0.35%, max -0.35%]
   Kraken/Bitfinex:     -0.39% [target  1.20%, min -0.39%, max -0.36%]
```

### Arbitrage Parameters

The two parameters used to control the arbitrage are `SpreadEntry` and `SpreadExit`.

* `SpreadEntry` is the limit above which a long/short trade is triggered.
* `SpreadExit` is the limit below which the long/short trade is closed.

`SpreadEntry` is actually the limit _after_ the exchange fees which means that `SpreadEntry` represents the net profit. If two exchanges have a 0.20% fees for every trade it means that for a full trade cycle we will have:

* 0.20% entry long + 0.20% entry short + 0.20% exit long + 0.20% exit short

In total we have 0.80% fees. The actual value is slightly different since it is a percentage of each actual trade but in our case it is close enough. Now if the profit we target is 0.30% (`"SpreadEntry": 0.0030`) then Blackbird will set the entry threshold at 1.10% (0.80% total fees + 0.30% target).

A smaller spread means more trades will be generated but with less profit each.


### Code Information

I am not a professional developer so there are parts that are not perfectly coded and you are welcome to help me out. However the arbitrage mechanism works and the system can generate profits.

Blackbird uses the base64 functions written by <a href="http://www.adp-gmbh.ch/cpp/common/base64.html" target="_target">René Nyffenegger</a> to encode and decode base64 in C++. Thank you René your code works well with Blackbird.

The results are stored in CSV files (e.g. _result_20150307_204649.csv_). A new CSV file is created every time Blackbird is started.

It is possible to properly stop Blackbird after the next trade has closed. While Blackbird is running just create an empty file called _stop_after_exit_ in the working directory. This will make Blackbird automatically stop after the next trade closes.


### How To Test Blackbird

#### Note

Please make sure you understand the disclaimer above if you are going to test the program with real money. You can start by testing with a limited amount of money like $20 per exchange. Note that it is never entirely safe to just tell the system to use only $20 per exchange (parameter `CashForTesting`). You also need to only have $20 available on each of your trading accounts so you will be sure than even with a bug with the `CashForTesting` parameter your maximum loss on an exchange won't be greater than $20 no matter what.

#### Credentials

You need to have accounts on Bitfinex, OKCoin and Bitstamp. Then with each account you need to create API authentication keys. This is usually in the _Settings_ section of your exchange accounts. In v0.0.2 the Kraken bid/ask information are displayed but the functions to send orders haven't been implemented yet.

The file that contains all the parameters is _config.json_. __Never__ share this file as it will contain your exchange credentials!

First add your credentials to _config.json_:

```json
"BitfinexApiKey"
"BitfinexSecretKey"
"OKCoinApiKey"
"OKCoinSecretKey"
"BitstampClientId"
"BitstampApiKey"
"BitstampSecretKey"
````

#### Strategy parameters

Modify the stategy parameters if needed. These are the ones by default:
```json
"SpreadEntry": 0.0030,
"SpreadExit": -0.0040,
```

#### Risk parameters

If you set `UseFullCash` at `true` then Blackbird will use the minimum cash on the accounts of your two trades, minus a small percentage defined by `UntouchedCash`.
For example, if you have:

```json
"UseFullCash": true,
"UntouchedCash": 0.01,
"CashForTesting": 20.00,
```

And let's say you have $1,000 on your Bitfinex trading account and $1,100 on your OKCoin trading account. The system will then use $990 on each exchange (i.e. $1,000 - 1%). So in this example your total exposure will be $1,980. Now if you change `UseFullCash` to `false` then the system will use $20 per exchange (total exposure $40).


### E-mail parameters (optional)

Blackbird can send you an automatic e-mail every time an arbitrage trade is done. The e-mails contain information like the trade performance. Here are the parameters:

```json
"SendEmail": false,
"SenderAddress": "john@example.com",
"SenderUsername": "john",
"SenderPassword": "pa33w0rd",
"SmtpServerAddress": "smtp.example.com:587",
"ReceiverAddress": "mark@example.com"
```

This feature is optional. If you let `SendEmail` to `false` you don't need to fill the other e-mail parameters.

#### Run the software

You need the following libraries: <a href="https://www.openssl.org/docs/crypto/crypto.html" target="_blank">Crypto</a>, <a href="http://www.digip.org/jansson" target="_blank">Jansson</a>, <a href="http://curl.haxx.se" target="_blank">cURL</a> and <a href="http://caspian.dotconf.net/menu/Software/SendEmail" target="_blank">sendEmail</a>.

For instance on Ubuntu you need to install the following libaries:
```
libssl-dev
libjansson-dev
libcurl4-openssl-dev
sendemail
```

Build the software by typing:
```
make
```

Then start the software:
```
./blackbird
```

Alternatively, you may want to start it in the background:

```
./blackbird > output.txt &
```

### Tasks And Issues

Please check the <a href="https://github.com/butor/blackbird/issues" target="_blank">issues page</a> for the current tasks and issues.

### Changelog

##### v0.0.1 - May 16th, 2015

* First release

##### v0.0.2 - May 31st, 2015

 * Bitstamp exchange added
 * Kraken exchange added (bid/ask information only, other functions to be implemented)
 * Improved JSON and cURL exceptions management
 * Added the milliseconds to the nonce used for exchange authentification
 * Several minor fixes and improvements

### Links

* <a href="https://bitcointalk.org/index.php?topic=985660.0" target="_blank">Discussion on BitcoinTalk</a>

### License

MIT
