#include <iostream>
#include <Web3.h>
#include <openssl/err.h>
#include <web3cpp/Provider.h>
#include <web3cpp/Account.h>
#include <web3cpp/Contract.h>
#include <web3cpp/ethcore/Common.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using json = nlohmann::ordered_json;

json loadABI(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open ABI file: " + filePath);
    }
    json abi;
    file >> abi;
    file.close();
    return abi;
}

int main(int argc, char** argv)
{
    auto p = Provider(
        "Anvil",
        "127.0.0.1",
        "/",
        8545,
        31337,
        "ETH",
        "",
        "ws"
    );

    auto web3 = Web3(p);

    Account acc1 = web3.wallet.createAccount("Account 1");
    Account acc2 = web3.wallet.createAccount("Account 2");

    std::cout << "Account 1 Address: " << acc1.address()
      << "\nPrivate Key" << acc1.privateKey() << "\n";

    std::cout << "Account 2 Address: " << acc2.address()
    << "\nPrivate Key" << acc2.privateKey() << "\n";

    auto acc1_balance = acc1.setBalance(Utils::toBN(Utils::toWei("6", 18))).get();
    auto acc2_balance = acc2.setBalance(Utils::toBN(Utils::toWei("3.5", 18))).get();

    std::cout << "Acccount 1 Balance: " << Utils::fromWei(acc1_balance, 18) << "\n";
    std::cout << "Acccount 2 Balance: " << Utils::fromWei(acc2_balance, 18) << "\n";

    std::cout << "Loading Contract: \n";
    json contract;
    json abi;
    std::string bytecode;

    try {
        contract = loadABI("../contracts/SHIFTTokenManager.json");
        abi = contract["abi"];
        bytecode = contract["bytecode"]["object"].get<std::string>();
    } catch (const std::exception& e)  {
        std::cerr << "Error loading contract: " << e.what() << "\n";
        return 1;
    }

    // {
    //     amount:
    //     destination:
    //     privateKey:
    //     etc...
    // }

    // SIGNER/SUBMITTER THREAD

    Error err;
    dev::eth::TransactionSkeleton trans_skel_1 = web3.wallet.buildTransaction(
        acc1.address(),
        acc1.nonce(),
        err,
        "",
        bytecode,
        Utils::toBN("0"),
        {}
    );

    if (err.getCode() != 0)  { std::cerr << "Error building transaction: " << err.what() << "\n"; }

    dev::eth::TransactionBase trans_base_1 = web3.wallet.estimateTransaction(trans_skel_1, dev::eth::FeeLevel::Medium, err);

    if (err.getCode() != 0)  { std::cerr << "Error estimating transaction: " << err.what() << "\n"; }

    std::string trans_signed_1 = web3.wallet.signTransaction(trans_base_1, acc1.privateKey(), err);

    if (err.getCode() != 0)  { std::cerr << "Error signing transaction: " << err.what() << "\n"; }

    auto res = web3.wallet.sendTransaction(trans_signed_1, err).get();

    if (err.getCode() != 0)  { std::cerr << "Error sending transaction: " << err.what() << "\n"; }

    std::string txHash = res["result"].get<std::string>();
    std::cout << "Deployment Transaction Hash: " << txHash << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));


    // POLLER THREAD
    auto receipt = web3.eth.getTransactionReceipt(txHash).get();
    std::cout << "Receipt: " << receipt.dump() << "\n";
    std::string contractAddress = receipt["result"]["contractAddress"].get<std::string>();
    std::cout << "Contract Address: " << contractAddress << "\n\n";

    std::cout << "Interacting with Contract: \n";

    Contract SHIFTTokenManager(abi, contractAddress);

    json deployFuncArgs = json::array();
    deployFuncArgs.push_back("MyToken");
    deployFuncArgs.push_back("MTK");
    deployFuncArgs.push_back("18");

    std::string encodedDeploy = SHIFTTokenManager(deployFuncArgs, "deploy", err);

    std::cout << "Made it here: " << encodedDeploy << "\n";

    if (err.getCode() != 0)  { std::cerr << "Error encoding deploy function: " << err.what() << "\n"; }

    dev::eth::TransactionSkeleton deployCallTxSkel = web3.wallet.buildTransaction(
        acc1.address(),
        acc1.nonce() + 1,  // Increment nonce
        err,
        contractAddress,
        encodedDeploy,
        Utils::toBN("0"),
        {}
    );

    dev::eth::TransactionBase deployCallTxBase = web3.wallet.estimateTransaction(
        deployCallTxSkel,
        dev::eth::FeeLevel::Medium,
        err
    );

    std::string deployCallTxSigned = web3.wallet.signTransaction(
        deployCallTxBase,
        acc1.privateKey(),
        err
    );

    auto deployCallRes = web3.wallet.sendTransaction(deployCallTxSigned, err).get();
    std::cout << "Token deployment transaction sent: " << deployCallRes["result"].get<std::string>() << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));

    json mintArgs = json::array();
    mintArgs.push_back("MyToken");
    mintArgs.push_back(acc2.address());
    mintArgs.push_back(Utils::toWei("100", 18));

    std::string encodedMint = SHIFTTokenManager(mintArgs, "mint", err);

    if (err.getCode() != 0) {
        std::cerr << "Error encoding mint function: " << err.what() << "\n";
        return 1;
    }

    dev::eth::TransactionSkeleton mintTxSkel = web3.wallet.buildTransaction(
        acc1.address(),
        acc1.nonce() + 2,  // Increment nonce again
        err,
        contractAddress,
        encodedMint,
        Utils::toBN("0"),
        {}
    );

    dev::eth::TransactionBase mintTxBase = web3.wallet.estimateTransaction(
        mintTxSkel,
        dev::eth::FeeLevel::Medium,
        err
    );

    std::string mintTxSigned = web3.wallet.signTransaction(
        mintTxBase,
        acc1.privateKey(),
        err
    );

    auto mintRes = web3.wallet.sendTransaction(mintTxSigned, err).get();
    std::cout << "Mint transaction sent: " << mintRes["result"].get<std::string>() << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));

    json balanceArgs = json::array();
    balanceArgs.push_back("MyToken");
    balanceArgs.push_back(acc2.address());

    std::string encodedBalance = SHIFTTokenManager(balanceArgs, "balance", err);

    if (err.getCode() != 0) {
        std::cerr << "Error encoding balance function: " << err.what() << "\n";
        return 1;
    }

    // Execute the call (read-only, no transaction)
    json callObj = json::object();
    callObj["to"] = contractAddress;
    callObj["data"] = encodedBalance;
    callObj["from"] = acc1.address();

    auto balanceRes = web3.eth.call(callObj).get();
    std::cout << "ERC-20 Balance of Account 2: " << Utils::fromWei(Utils::toBN(balanceRes["result"].get<std::string>()), 18) << "\n";

    std::cout << "Contract deployment and interaction completed successfully!\n";
}
