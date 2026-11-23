# tools/

Historical RPC node feed updater (`update_nodes.mjs`, Infura maps, auto-update
scripts) has been removed. Web3E no longer ships `src/nodes.h`.

**Devices obtain RPC endpoints from universal-node** via
`Web3::useCachedRpcPair(uninode)` / `uninode_getRpcPair` for any chainId uninode
serves (chainlist.org coverage). For lab work, use `Web3::setRpcUrl("https://…")`.
