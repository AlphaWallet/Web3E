#include <WiFi.h>
#include <Web3.h>
#include <Util.h>
#include <Contract.h>

//    To get this sample working correctly there is some preparation work you need to do.
//    Feel free to skip the majority of these instructions if you're an experienced ETH user - you probably just jump in at stage 6
//    If you already have Kovan or any other testnet ETH.
//
// 1. Choose an Ethereum test network (or if you're brave mainnet). I like Kovan as it's nice and fast. 
//    If you're using Infura you should sign up and use your own key. This is a test key I use, I wouldn't use it for anything serious.
// 2. Download a good ethereum wallet. The best one on Android and iOS is probably 'AlphaWallet' in the respective app stores. You
//    can also use MetaMask on desktop which works fine, but doesn't display the Non-Fungible Token standards such as ERC875 and ERC1155.
// 3. In Metamask ensure you created an account. If you already have an account with mainnet ETH in it create a new one.
//    Export the private key and import it into AlphaWallet (or just use the browser with Metamask). Also,
//    Copy/Paste the private key into the 'PRIVATE_KEY' constant below. This is why you shouldn't use any account containing real eth.
//    - you may accidentally commit the code to a public repo then you'll have all your eth gone very quickly.
// 4. Obtain some testnet Eth. If you're using Kovan there are two good faucets we know about:
//    https://gitter.im/kovan-testnet/faucet : requres a github account. Just find your address in your wallet,
//    and copy/paste it into the chat. The bot will pick it up and drop 3 ETH into your account.
//    https://faucet.kovan.network/ : also requires github account and will give you 1 ETH per day.
// 5. Ensure that you switch network to Koven in the wallet. Got to settings and click on 'Network'. Select Kovan.
// 6. Create an ERC20 contract. Once you gave some Kovan in your wallet open the DApp browser in AlphaWallet and go here:
//    http://thetokenfactory.com/#/factory Create a name and symbol for your token, 
//    choose 4 decimal places and a supply of say 5000000. This will give you 500 tokens in the account (50 0000 - 4 decimal places).
// 7. Click create and your wallet will receive a message to spend Kovan ETH to create the token.
// 8. Once the contract has been created and the tokens appear in your wallet copy/paste the contract address into ERC20CONTRACT below
// 9. To create ERC875 tokens go to this website in AlphaWallet or Metamask enabled desktop browser: 
//    https://alpha-wallet.github.io/ERC875-token-factory/index.html
//    and just fill in name and symbol then create. The tokens will be assigned to the creation address.
//    Once the tokens appear in your AlphaWallet, click on them and get the contract address (top right menu icon).
//    Paste the contract address in the 'ERC875CONTRACT' setting below.
//10. Set up a second account in you wallet and copy the Address (not private key!) into the 'TARGETADDRESS' setting below.
//    This way you get to catch all the sent tokens in your second account.


const char *ssid = "<Your SSID>";
const char *password = "<Your WiFi password>";
#define MY_ADDRESS "0xDA358D1547238335Cc666E318c511Fed455Ed77C"      //Put your wallet address here
#define ERC875CONTRACT "0x0181540116ea1047d7b5a22ce3394d5cecc0c3f1"  //Put your ERC875 contract address here
#define ERC20CONTRACT  "0x0ab0c2f54afe26cbd2ed2b56a27816e41092d040"  //Put your ERC20 contract address here
#define INFURA_HOST "kovan.infura.io"
#define INFURA_PATH "/v3/c7df4c29472d4d54a39f7aa78f146853"           //please change this Infura key to your own once you have the sample running
#define TARGETADDRESS "0x007bEe82BDd9e866b2bd114780a47f2261C684E3"   //put your second address here
#define ETHERSCAN_TX "https://kovan.etherscan.io/tx/"

// Copy/paste the private key from MetaMask in here
const char *PRIVATE_KEY = "DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00"; //32 Byte Private key 

int wificounter = 0;
Web3 web3(INFURA_HOST, INFURA_PATH);

void setup_wifi();
void PushERC875Transaction(); 
void queryERC875Balance(const char *userAddress);
void sendEthToAddress(double eth, const char *destination);
void PushERC20Transaction(double tokens, const char *toAddr, const char *contractAddr);

void setup() {
    Serial.begin(115200);

    setup_wifi();

    sendEthToAddress(0.1, TARGETADDRESS);
	PushERC20Transaction(0.1, TARGETADDRESS, ERC20CONTRACT);
    queryERC875Balance(MY_ADDRESS);
    PushERC20Transaction();
    PushERC875Transaction();
}

void loop() {

}

void sendEthToAddress(double eth, const char *destination)
{
	//obtain a contract object, for just sending eth doesn't need a contract address
	Contract contract(&web3, "");
	contract.SetPrivateKey(PRIVATE_KEY);
	unsigned long long gasPriceVal = 22000000000ULL;
	uint32_t  gasLimitVal = 90000;
	string address = MY_ADDRESS;

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

void PushERC20Transaction(double tokens, const char *toAddr, const char *contractAddr) 
{
	string contractAddrStr = contractAddr;
	Contract contract(&web3, contractAddr);
	contract.SetPrivateKey(<Your private key>);

	//Get contract name (This isn't needed to push the transaction)
	string param = contract.SetupContractData("name()", &addr);
	string result = contract.ViewCall(&param);
	string interpreted = Util::InterpretStringResult(web3.getString(&result).c_str());
	Serial.println(interpreted.c_str());

	//Get Contract decimals
	param = contract.SetupContractData("decimals()", &addr);
	result = contract.ViewCall(&param);
	int decimals = web3.getInt(&result);
	Serial.println(decimals);

	unsigned long long gasPriceVal = 22000000000ULL;
	uint32_t  gasLimitVal = 4300000;

	//amount of erc20 token to send, note we use decimal value obtained earlier
	uint256_t weiValue = Util::ConvertToWei(tokens, decimals);

	//get nonce
	uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&addr);
	string toAddress = toAddr;
	string valueStr = "0x00";

	//Setup contract function call
	string p = contract.SetupContractData("transfer(address,uint256)", &toAddress, &weiValue); //ERC20 function plus params

	//push transaction to ethereum
	result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &contractAddrStr, &valueStr, &p);
	string transactionHash = web3.getString(&result);
}


/* Query balance of ERC875 tokens */
void queryERC875Balance(const char *userAddress)
{
    // transaction
    Contract contract(&web3, ERC875CONTRACT);

    String myAddr = userAddress;

    String func = "balanceOf(address)";
    Serial.println("Start");
    string param = contract.SetupContractData(func.c_str(), &myAddr);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    Serial.print("Balance of Contract ");
    Serial.println(ERC875CONTRACT);
    Serial.print("for user: ");
    Serial.println(myAddr.c_str());
    Serial.println();

    //break down the result
    vector<string> *vectorResult = Util::InterpretVectorResult(&result);
    int count = 0;
    char buffer[128];
    for (auto itr = vectorResult->begin(); itr != vectorResult->end(); itr++)
    {
        snprintf(buffer, 128, "%d: %s", count++, itr->c_str());
        Serial.println(buffer);
    }

    delete(vectorResult);

    //Call contract name function
    param = contract.SetupContractData("name()", &userAddress);
    result = contract.ViewCall(&param);
    string contractName = web3.getString(&result);
    Serial.println("NAME: ");
    // recover actual string name
    string interpreted = Util::InterpretStringResult(contractName.c_str());
    Serial.println(interpreted.c_str());
}

void PushERC875Transaction()
{
    Contract contract(&web3, ERC875CONTRACT);
    contract.SetPrivateKey(PRIVATE_KEY);
    string addr = MY_ADDRESS;
    uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&addr);

    unsigned long long gasPriceVal = 22000000000ULL;
    uint32_t  gasLimitVal = 4300000;
    uint8_t dataStr[100];
    memset(dataStr, 0, 100);
    string address = TARGETADDRESS;
    vector<uint32_t> indices;
    indices.push_back(1); // transfer NFT index 1 (ie the second token, since index 0 is the first token)
    string p = contract.SetupContractData("transfer(address,uint256[])", &address, &indices);
    string contractAddr = ERC875CONTRACT;
    string valueStrThis = "0x00";

    string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &contractAddr, &valueStrThis, &p);
    Serial.println(result.c_str());
    string transactionHash = web3.getString(&result);
    Serial.println("TX on Etherscan:");
    Serial.print(ETHERSCAN_TX);
    Serial.println(transactionHash.c_str()); //you can go straight to etherscan and see the pending transaction
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