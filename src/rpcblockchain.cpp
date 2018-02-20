// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "bitcoinrpc.h"

#ifndef QT_GUI
#include "util.h"
#include "init.h"

#include "miniunz.h"
#include <curl/curl.h>
#endif

using namespace json_spirit;
using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);

mp_float GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return mp_float(1);
        else
            blockindex = GetLastBlockIndex(pindexBest, false);
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    mp_float dDiff =
        mp_float(0x0000ffff) / mp_float(blockindex->nBits & 0x00ffffff);
    while (nShift < 29)
    {
        dDiff *= 256;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256;
        nShift--;
    }
    return dDiff;
}

double GetPoWMHashPS()
{
    if (pindexBest->nHeight >= LAST_POW_BLOCK)
        return 0;

    int nPoWInterval = 72;
    int64_t nTargetSpacingWorkMin = 30, nTargetSpacingWork = 30;

    CBlockIndex* pindex = pindexGenesisBlock;
    CBlockIndex* pindexPrevWork = pindexGenesisBlock;

    while (pindex)
    {
        if (pindex->IsProofOfWork())
        {
            int64_t nActualSpacingWork = pindex->GetBlockTime() - pindexPrevWork->GetBlockTime();
            nTargetSpacingWork = ((nPoWInterval - 1) * nTargetSpacingWork + nActualSpacingWork + nActualSpacingWork) / (nPoWInterval + 1);
            nTargetSpacingWork = max(nTargetSpacingWork, nTargetSpacingWorkMin);
            pindexPrevWork = pindex;
        }

        pindex = pindex->pnext;
    }

    return GetDifficulty().convert_to<double>() * 4294.967296 / nTargetSpacingWork;
}

mp_float GetPoSKernelPS(CBlockIndex* pindexPrev)
{
    int nPoSInterval = 72;
    mp_float dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    CBlockIndex* pindexPrevStake = NULL;

    while (pindexPrev && nStakesHandled < nPoSInterval)
    {
        if (pindexPrev->IsProofOfStake())
        {
            dStakeKernelsTriedAvg += GetDifficulty(pindexPrev) * UINT64_C(4294967296);
            nStakesTime += pindexPrevStake ? (pindexPrevStake->nTime - pindexPrev->nTime) : 0;
            pindexPrevStake = pindexPrev;
            nStakesHandled++;
        }

        pindexPrev = pindexPrev->pprev;
    }

   return nStakesTime ? dStakeKernelsTriedAvg / nStakesTime : 0;
}

mp_float GetPoSKernelPS()
{
    return GetPoSKernelPS(pindexBest);
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool fPrintTransactionDetail)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex).convert_to<double>()));
    result.push_back(Pair("blocktrust", leftTrim(blockindex->GetBlockTrust().GetHex(), '0')));
    result.push_back(Pair("chaintrust", leftTrim(blockindex->nChainTrust.GetHex(), '0')));
    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", blockindex->IsProofOfStake()? blockindex->hashProofOfStake.GetHex() : blockindex->GetBlockHash().GetHex()));
    result.push_back(Pair("entropybit", (int)blockindex->GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016" PRIx64, blockindex->nStakeModifier)));
    result.push_back(Pair("modifierchecksum", strprintf("%08x", blockindex->nStakeModifierChecksum)));
    Array txinfo;
    BOOST_FOREACH (const CTransaction& tx, block.vtx)
    {
        if (fPrintTransactionDetail)
        {
            Object entry;

            entry.push_back(Pair("txid", tx.GetHash().GetHex()));
            TxToJSON(tx, 0, entry);

            txinfo.push_back(entry);
        }
        else
            txinfo.push_back(tx.GetHash().GetHex());
    }

    result.push_back(Pair("tx", txinfo));

    if (block.IsProofOfStake())
        result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}

Value getbestblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "Returns the hash of the best block in the longest block chain.");

    return hashBestChain.GetHex();
}

Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}


Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the difficulty as a multiple of the minimum difficulty.");

    Object obj;
    obj.push_back(Pair("proof-of-work",        GetDifficulty().convert_to<double>()));
    obj.push_back(Pair("proof-of-stake",       GetDifficulty(GetLastBlockIndex(pindexBest, true)).convert_to<double>()));
    obj.push_back(Pair("search-interval",      (int)nLastCoinStakeSearchInterval));
    return obj;
}


Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1 || AmountFromValue(params[0]) < MIN_TX_FEE)
        throw runtime_error(
            "settxfee <amount>\n"
            "<amount> is a real and is rounded to the nearest 0.01");

    nTransactionFee = AmountFromValue(params[0]);
    nTransactionFee = (nTransactionFee / CENT) * CENT;  // round to cent

    return true;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <hash> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

Value getblockbynumber(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <number> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-number.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    uint256 hash = *pblockindex->phashBlock;

    pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

// ppcoin: get information of sync-checkpoint
Value getcheckpoint(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getcheckpoint\n"
            "Show info of synchronized checkpoint.\n");

    Object result;
    const CBlockIndex* pindexCheckpoint = Checkpoints::AutoSelectSyncCheckpoint();

    result.push_back(Pair("synccheckpoint", pindexCheckpoint->GetBlockHash().ToString().c_str()));
    result.push_back(Pair("height", pindexCheckpoint->nHeight));
    result.push_back(Pair("timestamp", DateTimeStrFormat(pindexCheckpoint->GetBlockTime()).c_str()));
    result.push_back(Pair("policy", "rolling"));

    return result;
}

#ifndef QT_GUI
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

int DownloadFile(std::string url, boost::filesystem::path target_file_path)
{
    int err = 0;

    printf("bootstrap: Downloading blockchain from %s. \n", url.c_str());

    CURL *curlHandle = curl_easy_init();
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);

    FILE *file = fopen(target_file_path.c_str(), "wb");
    if(file)
    {
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, file);
        CURLcode curl_err = curl_easy_perform(curlHandle);
        if (curl_err != CURLE_OK)
            printf("bootstrap: Error downloading from %s. Error: %s.\n", url.c_str(), curl_easy_strerror(curl_err));
        fclose(file);
        err = (int)curl_err;
    }
    else
    {
        printf("bootstrap: Download error: Unable to open output file for writing: %s.\n", target_file_path.c_str());
        err = -1;
    }

    curl_easy_cleanup(curlHandle);

    return err;
}

int ExtractBootstrapFile(boost::filesystem::path& pathBootstrap)
{
    printf("bootstrap: Extracting bootstrap file\n");
    if (!boost::filesystem::exists(pathBootstrap)) {
        printf("bootstrap: Bootstrap file doesn't exist!\n");
        return -1;
    }

    const char * zipfilename = pathBootstrap.c_str();
    unzFile uf;
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64A(&ffunc);
    uf = unzOpen2_64(zipfilename, &ffunc);
#else
    uf = unzOpen64(zipfilename);
#endif

    if (uf == NULL)
    {
        printf("bootstrap: Cannot open downloaded file: %s\n", zipfilename);
        return -2;
    }

    int unzip_err = zip_extract_all(uf, GetDataDir(), "bootstrap");
    if (unzip_err != UNZ_OK)
    {
        printf("bootstrap: Unzip failed\n");
        return -3;
    }

    printf("bootstrap: Unzip successful\n");

    if (!boost::filesystem::exists(GetDataDir() / "bootstrap" / "blk0001.dat") ||
        !boost::filesystem::exists(GetDataDir() / "bootstrap" / "txleveldb"))
    {
        printf("bootstrap: Downloaded zip file did not contain all necessary files!\n");
        return -4;
    }

    return 0;
}

Value bootstrap(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "bootstrap [overwrite_roxycoin_conf=false]\n"
            "Download blockchain and optionally a current roxycoin.conf from www.roxycoin.info.\n"
            "Daemon exits when finished."
        );

    if (params.size() == 1)
    {
        fBootstrapConfig = params[0].get_bool();
    }

    Object result;
    boost::filesystem::path pathBootstrapZip = GetDataDir() / "bootstrap.zip";
    int err = DownloadFile("https://www.roxycoin.info/downloads/bootstrap.zip", pathBootstrapZip);
    if (err != 0)
    {
        printf("bootstrap: Download failed!\n");
        result.push_back(Pair("success", false));
        result.push_back(Pair("error", "Download failed"));
        result.push_back(Pair("error_code", err));
        return result;
    }
    printf("bootstrap: Download successful\n");

    err = ExtractBootstrapFile(pathBootstrapZip);
    if (err != 0)
    {
        printf("bootstrap: Extracting failed!\n");
        result.push_back(Pair("success", false));
        result.push_back(Pair("error", "Extracting failed"));
        result.push_back(Pair("error_code", err));
        return result;
    }

    fBootstrapTurbo = true;
    StartShutdown();

    result.push_back(Pair("success", true));
    result.push_back(Pair("comment", "Bootstrap successful; roxycoind has been stopped, please restart."));

    return result;
}
#endif

