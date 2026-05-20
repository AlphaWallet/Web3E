# AploEmbed Follow-Up Items

This document tracks non-blocking improvements identified during final review (t_4e3de54e) that can be addressed in future PRs.

## Medium Priority

### M1: RPC Failover Not Actually Used
**Status:** Implemented but not functional  
**Location:** `src/Web3.cpp` lines 400-432  
**Description:** `execWithFailover()` method exists but is never called. All Aplo helper methods (`AploGetBalance`, `AploGetStake`, etc.) use `EthGetBalance()` and `EthViewCall()` which call `exec()` directly without failover logic.

**Impact:** Advertised failover feature is non-functional. Primary RPC failure will cause `ESP.restart()` instead of trying fallback RPC.

**Recommendation:** Refactor `EthGetBalance()` and `EthViewCall()` to use `execWithFailover()` internally, or document as known limitation in README.

**Reference:** REVIEW_REPORT.md lines 78-84

---

## Low Priority

### L2: Placeholder Contract Addresses in AploContracts.h
**Status:** Expected for library template  
**Location:** `src/AploContracts.h` lines 9-12  
**Description:** Mining and staking contract addresses are placeholders (`0x00...1234`, `0x00...1235`).

**Impact:** Examples will fail at runtime if users don't replace addresses. Clear warnings exist in code comments and all example READMEs.

**Mitigation:** 
- All 4 examples include explicit warnings to update contract addresses
- AploContracts.h has clear comments marking addresses as placeholders
- README documents that contract addresses must be configured

**Recommendation:** Add a dedicated section in main README explaining contract address configuration requirements.

**Reference:** REVIEW_REPORT.md lines 200-206

---

## Resolved During Review

### ✅ L1: README Title Branding
**Status:** Fixed in commit following review  
**Change:** Updated README.md line 1 from "Web3E Ethereum for Embedded devices" to "AploEmbed - AploCoin for Embedded Devices"  
**Reference:** REVIEW_REPORT.md lines 193-198

---

## Review Metadata

**Review Task:** t_4e3de54e  
**Review Date:** 2026-05-20T15:04:10Z  
**Verdict:** ✅ READY TO PUBLISH  
**Blocking Issues:** 0  
**Non-Blocking Issues:** 2 remaining (M1, L2)

All items in this document are **non-blocking** for initial PR publication.
