#include <WiFi.h>
#include <Web3.h>
#include <Util.h>
#include <Contract.h>

//    To get this sample working correctly there is some preparation work you need to do.
//    Feel free to skip the majority of these instructions if you're an experienced ETH user - you probably just jump in at stage 6
//    If you already have Sepolia or any other testnet ETH.
//
// 1. Choose an Ethereum test network (or if you're brave mainnet). Sepolia or Mumbai are currently the best testnets. Let's use Sepolia in this example
//      Sepolia faucet is here: https://sepoliafaucet.com/ You will need to log in with gmail or github account. It gives 0.5/day Sepolia which is plenty for testing
//    If you're using Infura you should sign up and use your own key. I have bundled a test key, don't use this for production as it has a small daily quota.
// 2. Download a good ethereum wallet. The best one on Android and iOS is probably 'AlphaWallet' in the respective app stores. You
//    can also use MetaMask on desktop which works fine, but is limited in use and isn't very safe. It doesn't support TokenScript which we will be using later.
// 3. In your wallet ensure you created an account. If you already have an account with mainnet ETH in it create a new one for testing.
//    Export the private key and import it into AlphaWallet (or just use the browser with Metamask). Also,
//    Copy/Paste the private key into the 'PRIVATE_KEY' constant below. This is why you shouldn't use any account containing real eth.
//    - you may accidentally commit the code to a public repo then you'll have all your eth gone very quickly (ask me how I know).
// 4. Obtain some testnet Eth from the faucet above.
// 5. Ensure that you switch network to Sepolia in the wallet. Got to settings and click on 'Network'. Select Sepolia.
// 6. Create an ERC20 contract. Once you gave some Sepolia in your wallet open the DApp browser in AlphaWallet and go here:
//    https://www.smartcontracts.tools/token-generator/create/ethereum/ Create a name and symbol for your token, choose Sepolia under network
//    Select the name you want, Initial supply etc - it doesn't matter for the test.
// 7. Click create and your wallet will receive a message to spend Sepolia ETH to create the token.
// 8. Once the contract has been created and the tokens appear in your wallet copy/paste the contract address into ERC20CONTRACT below
// 9. Set up a second account in you wallet and copy the Address (not private key!) into the 'TARGETADDRESS' setting below.
//    This way you get to catch all the sent tokens in your second account.


const char *ssid = "< your WiFi SSID >";
const char *password = "< your WiFi password >";
#define MY_ADDRESS "0x5F89E7FBa60ff6a98fB002c1854E8f29C8b446Cf"      //Put your wallet address here
#define ERC875CONTRACT "0x0181540116ea1047d7b5a22ce3394d5cecc0c3f1"  //Put your ERC875 contract address here
#define ERC20CONTRACT  "0xd46c787493418a6a55bd2e01f2a3d598c1a0da08"  //Put your ERC20 contract address here
#define TARGETADDRESS "0xA20efc4B9537d27acfD052003e311f762620642D"   //put your second address here
#define VITALIKADDRESS "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"
#define CHILLIFROGS "0xa3b7cee4e082183e69a03fc03476f28b12c545a7"
#define ETHERSCAN_TX "https://sepolia.etherscan.io/tx/"

// Copy/paste the private key from MetaMask in here
const char *PRIVATE_KEY = "DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00DabbaD00"; //32 Byte Private key 

int wificounter = 0;
Web3 *web3;

void setup_wifi();
void PushERC875Transaction(); 
void queryERC721Balance(const char *userAddress);
void sendEthToAddress(double eth, const char *destination);
void PushERC20Transaction(double tokens, const char *toAddr, const char *contractAddr);

void setup() {
    Serial.begin(115200);
    web3 = new Web3(SEPOLIA_ID);

    setup_wifi();

    sendEthToAddress(0.01, TARGETADDRESS);
	PushERC20Transaction(10, TARGETADDRESS, ERC20CONTRACT);
    queryERC721Balance(MY_ADDRESS);
}

void loop() {

}

void sendEthToAddress(double eth, const char *destination)
{
	//obtain a contract object, for just sending eth doesn't need a contract address
	Contract contract(web3, "");
	contract.SetPrivateKey(PRIVATE_KEY);
	unsigned long long gasPriceVal = 2200000000LL;//22000000000ULL; //Probably should get the current gas price here!
	uint32_t  gasLimitVal = 23000;
	string address = MY_ADDRESS;

	//convert eth value to Wei
	uint256_t weiValue = Util::ConvertToWei(eth, 18);
	string emptyString = "";
	string destinationAddress = destination;

	Serial.print("Get Nonce: ");
	uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&address);
	Serial.println(nonceVal);
	Serial.println("Send TX");
	string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &destinationAddress, &weiValue, &emptyString);
	Serial.println(result.c_str());

	string transactionHash = web3->getResult(&result);
	Serial.println("TX on Etherscan:");
	Serial.print(ETHERSCAN_TX);
	Serial.println(transactionHash.c_str()); //you can go straight to etherscan and see the pending transaction
}

void PushERC20Transaction(double tokens, const char *toAddr, const char *contractAddr) 
{
	string contractAddrStr = contractAddr;
	Contract contract(web3, contractAddr);
	contract.SetPrivateKey(PRIVATE_KEY);
    string address = MY_ADDRESS;

	//Get contract name (This isn't needed to push the transaction)
	string param = contract.SetupContractData("name()", &toAddr);
	string result = contract.ViewCall(&param);
	string interpreted = Util::InterpretStringResult(web3->getString(&result).c_str());
	Serial.println(interpreted.c_str());

	//Get Contract decimals
	param = contract.SetupContractData("decimals()", &toAddr);
	result = contract.ViewCall(&param);
	int decimals = web3->getInt(&result);
	Serial.println(decimals);

	unsigned long long gasPriceVal = 2200000000LL;
	uint32_t  gasLimitVal = 4300000;

	//amount of erc20 token to send, note we use decimal value obtained earlier
	uint256_t weiValue = Util::ConvertToWei(tokens, decimals);

	//get nonce
    Serial.print("Get Nonce: ");
    string toAddress = toAddr;
	uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&address);
	uint256_t callValue = 0;
    Serial.println(nonceVal);
	Serial.println("Send TX");

	//Setup contract function call
	string p = contract.SetupContractData("transfer(address,uint256)", &toAddress, &weiValue); //ERC20 function plus params

	//push transaction to ethereum
	result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &contractAddrStr, &callValue, &p);
    Serial.println(result.c_str());
	string transactionHash = web3->getResult(&result);

    Serial.println("TX on Etherscan:");
	Serial.print(ETHERSCAN_TX);
	Serial.println(transactionHash.c_str()); //you can go straight to etherscan and see the pending transaction
}


/* Query balance of ERC721 tokens on vitalik's hot wallet */
void queryERC721Balance(const char *userAddress)
{
    delete (web3);
    web3 = new Web3(MAINNET_ID);
    // transaction
    
    Contract contract(web3, CHILLIFROGS);


    string myAddr = VITALIKADDRESS;
    string frogs = CHILLIFROGS;

    string func = "balanceOf(address)";
    Serial.println("Start");
    string param = contract.SetupContractData(func.c_str(), &myAddr);
    string result = contract.ViewCall(&param);

    Serial.println(result.c_str());

    Serial.print("Balance of Contract ");
    Serial.println(CHILLIFROGS);
    Serial.print("for user: ");
    Serial.print(myAddr.c_str());
    Serial.print(" -> ");

    long bal = web3->getInt(&result);

    Serial.println(bal);

    Serial.println("Fetch Name ... ");

    //Call contract name function
    param = contract.SetupContractData("name()");
    result = contract.ViewCall(&param);
    string contractName = web3->getString(&result);
    Serial.print("NAME: ");
    Serial.println(contractName.c_str());
}

//LEGACY NO LONGER USED (ERC-875 is dead)
/*void PushERC875Transaction()
{
    Contract contract(web3, ERC875CONTRACT);
    contract.SetPrivateKey(PRIVATE_KEY);
    string addr = MY_ADDRESS;
    uint32_t nonceVal = (uint32_t)web3->EthGetTransactionCount(&addr);

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
    string transactionHash = web3->getString(&result);
    Serial.println("TX on Etherscan:");
    Serial.print(ETHERSCAN_TX);
    Serial.println(transactionHash.c_str()); //you can go straight to etherscan and see the pending transaction
}*/


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