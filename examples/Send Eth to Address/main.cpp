#include <WiFi.h>
#include <Web3.h>
#include <Util.h>
#include <Contract.h>

//    To get this sample working correctly there is some preparation work you need to do.
//    Feel free to skip the majority of these instructions if you're an experienced ETH user - you probably just need 
//    to fill in your private key and accounts
//
// 1. Choose an Ethereum test network (or if you're brave mainnet). I like Kovan as it's nice and fast. 
//    If you're using Infura you should sign up and use your own key. This is a test key I use, I wouldn't use it for anything serious.
// 2. Download a good ethereum wallet. The best one on Android and iOS is probably 'AlphaWallet' in the respective app stores. You
//    can also use MetaMask on desktop which works fine, but doesn't display the Non-Fungible Token standards such as ERC875 and ERC1155.
// 3. In Metamask ensure you created an account. If you already have an account with mainnet ETH in it create a new one.
//    Export the private key and import it into AlphaWallet (or just use the browser with Metamask). Also,
//    Copy/Paste the private key into the 'PRIVATE_KEY' constant below. This is why you shouldn't use any account containing real eth for testing.
//    - you may accidentally commit the code to a public repo then you'll have all your eth gone very quickly.
// 4. Obtain some testnet Eth. If you're using Kovan there are two good faucets we know about:
//    https://gitter.im/kovan-testnet/faucet : requres a github account. Just find your address in your wallet,
//    and copy/paste it into the chat. The bot will pick it up and drop 3 ETH into your account.
//    https://faucet.kovan.network/ : also requires github account and will give you 1 ETH per day.
// 5. Ensure that you switch network to Koven in the wallet. Got to settings and click on 'Network'. Select Kovan.
// 6. Set up a second account in you wallet and copy the Address (not private key!) into the 'TARGETADDRESS' setting below.
//    This way you get to catch all the sent ETH in your second account.


const char *ssid = "<Your SSID>";
const char *password = "<Your WiFi password>";
#define MY_ADDRESS "0xDA358D1547238335Cc666E318c511Fed455Ed77C"      //Put your wallet address here
#define INFURA_HOST "kovan.infura.io"
#define INFURA_PATH "/v3/c7df4c29472d4d54a39f7aa78f146853"           //please change this Infura key to your own once you have the sample running
#define TARGETADDRESS "0x007bEe82BDd9e866b2bd114780a47f2261C684E3"   //put your second address here
#define ETHERSCAN_TX "https://kovan.etherscan.io/tx/"

// Copy/paste the private key from MetaMask in here
const char *PRIVATE_KEY = "DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00"; //32 Byte Private key 

int wificounter = 0;
Web3 web3(INFURA_HOST, INFURA_PATH);

void setup_wifi();
void sendEthToAddress(double eth, const char *destination); 
double queryAccountBalance(const char *address);

void setup() {
    Serial.begin(115200);

    setup_wifi();

    double balance = queryAccountBalance(MY_ADDRESS);
    Serial.print("My Address: ");
    Serial.println(MY_ADDRESS);
    Serial.print("Balance of Address: ");
    Serial.println(balance);

    Serial.println("Try to send 0.1 ETH");
    Serial.print("To Address: ");
    Serial.println(TARGETADDRESS);

    if (balance < 0.11) //allow for gas
    {
        sendEthToAddress(0.1, TARGETADDRESS);
    }
    else
    {
        Serial.println("Balance is insufficient, cannot send ... Go to Eth faucet.");
    }
}

double queryAccountBalance(const char *address)
{
	uint256_t balance = web3.EthGetBalance(&address); //obtain balance in Wei
	string balanceStr = Util::ConvertWeiToEthString(&balance, 18); //Eth uses 18 decimal
	Serial.print("Balance: ");
	Serial.println(balanceStr.c_str());
	double balanceDbl = atof(balanceStr.c_str());
	return balanceDbl;
}

void sendEthToAddress(double eth, const char *destination)
{
	//obtain a contract object, for just sending eth doesn't need a contract address
	Contract contract(&web3, "");
	contract.SetPrivateKey(PRIVATE_KEY);
	unsigned long long gasPriceVal = 22000000000ULL;
	uint32_t  gasLimitVal = 90000;

	//convert eth value to Wei
	uint256_t weiValue = Util::ConvertToWei(eth, 18);
	string emptyString = "";
	string destinationAddress = destination;

	Serial.print("Get Nonce: ");
	uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&address);
	Serial.println(nonceVal);
	Serial.println("Send TX");
	string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &destinationAddress, &weiValue, &emptyString);
	Serial.println(result.c_str());

	string transactionHash = web3.getString(&result);
	Serial.println("TX on Etherscan:");
	Serial.print(ETHERSCAN_TX);
	Serial.println(transactionHash.c_str()); //you can go straight to etherscan and see the pending transaction
}

void loop() {

}


/* This routine is specifically geared for ESP32 perculiarities */
/* You may need to change the code as required */
/* It should work on 8266 as well */
void setup_wifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);

        WiFi.begin(ssid, password);
    }

    wificounter = 0;
    while (WiFi.status() != WL_CONNECTED && wificounter < 10)
    {
        for (int i = 0; i < 500; i++)
        {
            delay(1);
        }
        Serial.print(".");
        wificounter++;
    }

    if (wificounter >= 10)
    {
        Serial.println("Restarting ...");
        ESP.restart(); //targetting 8266 & Esp32 - you may need to replace this
    }

    delay(10);

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}