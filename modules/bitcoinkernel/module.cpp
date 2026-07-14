#include "module.h"
#include <kernel/bitcoinkernel_wrapper.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>

namespace {
std::string bytes_to_hex(const uint8_t *bytes, int length) {
  std::stringstream string_stream;
  string_stream << std::hex;
  for (int i = 0; i < length; ++i) {
    string_stream << std::setw(2) << std::setfill('0')
                  << static_cast<int>(bytes[i]);
  }

  return string_stream.str();
}

std::string hash_bytes_to_hex(const uint8_t *bytes, int length) {
  std::stringstream string_stream;
  string_stream << std::hex;
  for (int i = length - 1; i >= 0; --i) {
    string_stream << std::setw(2) << std::setfill('0')
                  << static_cast<int>(bytes[i]);
  }

  return string_stream.str();
}

btck::ChainType decode_chain_type(uint8_t value) {
  constexpr std::array chain_types{
      btck::ChainType::MAINNET, btck::ChainType::TESTNET,
      btck::ChainType::TESTNET_4, btck::ChainType::SIGNET,
      btck::ChainType::REGTEST};
  return chain_types[value % chain_types.size()];
}

std::string chain_type_to_string(btck::ChainType chain_type) {
  switch (chain_type) {
  case btck::ChainType::MAINNET:
    return "mainnet";
  case btck::ChainType::TESTNET:
    return "testnet";
  case btck::ChainType::TESTNET_4:
    return "testnet4";
  case btck::ChainType::SIGNET:
    return "signet";
  case btck::ChainType::REGTEST:
    return "regtest";
  }

  assert(false);
}

btck::BlockCheckFlags decode_block_check_flags(uint8_t value) {
  auto flags = btck::BlockCheckFlags::BASE;
  if ((value & 0x01) != 0)
    flags |= btck::BlockCheckFlags::POW;
  if ((value & 0x02) != 0)
    flags |= btck::BlockCheckFlags::MERKLE;
  return flags;
}

char *libbitcoinkernel_transaction(std::span<const uint8_t> buffer) {
  try {
    std::span<const std::byte> raw_span{(const std::byte *)buffer.data(),
                                        buffer.size()};
    btck::Transaction transaction{raw_span};

    const auto txid_bytes = transaction.Txid().ToBytes();
    std::string result = "txid=";
    result.append(hash_bytes_to_hex((const uint8_t *)txid_bytes.data(),
                                    static_cast<int>(txid_bytes.size())));
    result.append(";");

    const auto txins = transaction.Inputs();
    for (const auto &txin : txins) {
      const auto outpoint = txin.OutPoint();
      uint32_t outpoint_index = outpoint.index();
      const auto outpoint_txid_bytes = outpoint.Txid().ToBytes();
      result.append("index=");
      result.append(std::to_string(outpoint_index));
      result.append("txid=");
      result.append(
          hash_bytes_to_hex((const uint8_t *)outpoint_txid_bytes.data(),
                            static_cast<int>(outpoint_txid_bytes.size())));
      result.append(";");
    }

    const auto txouts = transaction.Outputs();
    for (const auto &txout : txouts) {
      int64_t txout_amount = txout.Amount();
      const auto script_pubkey_bytes = txout.GetScriptPubkey().ToBytes();
      result.append("amount=");
      result.append(std::to_string(txout_amount));
      result.append("script_pubkey=");
      result.append(bytes_to_hex((const uint8_t *)script_pubkey_bytes.data(),
                                 static_cast<int>(script_pubkey_bytes.size())));
      result.append(";");
    }
    return strdup(result.c_str());
  } catch (...) {
    return strdup("0");
  }
}

char *libbitcoinkernel_block(std::span<const uint8_t> buffer) {
  try {
    std::span<const std::byte> raw_span{(const std::byte *)buffer.data(),
                                        buffer.size()};
    btck::Block block{raw_span};

    const auto block_hash_bytes = block.GetHash().ToBytes();
    std::string result =
        hash_bytes_to_hex((const uint8_t *)block_hash_bytes.data(),
                          static_cast<int>(block_hash_bytes.size()));

    const auto txs = block.Transactions();
    for (const auto &tx : txs) {
      const auto txid_bytes = tx.Txid().ToBytes();
      result.append("txid=");
      result.append(hash_bytes_to_hex((const uint8_t *)txid_bytes.data(),
                                      static_cast<int>(txid_bytes.size())));
      result.push_back(';');
    }
    return strdup(result.c_str());

  } catch (...) {
    return strdup("0");
  }
}

char *libbitcoinkernel_block_check(std::span<const uint8_t> buffer) {
  const uint8_t chain_selector = buffer.size() > 0 ? buffer[0] : 0;
  const uint8_t flag_selector = buffer.size() > 1 ? buffer[1] : 0;
  const auto chain_type = decode_chain_type(chain_selector);
  const auto flags = decode_block_check_flags(flag_selector);
  const auto raw_block = buffer.subspan(
      buffer.size() > 2 ? static_cast<size_t>(2) : buffer.size());

  std::string result = "chain=";
  result.append(chain_type_to_string(chain_type));
  result.append(";flags=");
  result.append(std::to_string(flag_selector & 0x03));
  result.push_back(';');

  try {
    btck::ChainParams chain_params{chain_type};
    std::span<const std::byte> raw_span{(const std::byte *)raw_block.data(),
                                        raw_block.size()};
    btck::Block block{raw_span};
    btck::BlockValidationState state{};
    const bool ok =
        block.Check(chain_params.GetConsensusParams(), flags, state);

    result.append("ok=");
    result.append(ok ? "1" : "0");
    result.append(";mode=");
    result.append(std::to_string(static_cast<int>(state.GetValidationMode())));
    result.append(";result=");
    result.append(
        std::to_string(static_cast<int>(state.GetBlockValidationResult())));
    result.append(";hash=");
    const auto block_hash_bytes = block.GetHash().ToBytes();
    result.append(hash_bytes_to_hex((const uint8_t *)block_hash_bytes.data(),
                                    static_cast<int>(block_hash_bytes.size())));
    result.append(";txs=");
    result.append(std::to_string(block.CountTransactions()));
    result.push_back(';');
    return strdup(result.c_str());
  } catch (...) {
    result.append("err=exception;");
    return strdup(result.c_str());
  }
}
} // namespace

namespace bitcoinfuzz {
namespace module {
BitcoinKernel::BitcoinKernel(void) : BaseModule("BitcoinKernel") {}

std::optional<std::string>
BitcoinKernel::kernel_transaction(std::span<const uint8_t> buffer) const {
  auto result_ptr = libbitcoinkernel_transaction(buffer);
  if (result_ptr == nullptr)
    return std::nullopt;

  std::string result(result_ptr);
  free(result_ptr);
  return result;
}

std::optional<std::string>
BitcoinKernel::kernel_block(std::span<const uint8_t> buffer) const {
  auto result_ptr = libbitcoinkernel_block(buffer);
  if (result_ptr == nullptr)
    return std::nullopt;

  std::string result(result_ptr);
  free(result_ptr);
  return result;
}

std::optional<std::string>
BitcoinKernel::kernel_block_check(std::span<const uint8_t> buffer) const {
  auto result_ptr = libbitcoinkernel_block_check(buffer);
  if (result_ptr == nullptr)
    return std::nullopt;

  std::string result(result_ptr);
  free(result_ptr);
  return result;
}

} // namespace module
} // namespace bitcoinfuzz
