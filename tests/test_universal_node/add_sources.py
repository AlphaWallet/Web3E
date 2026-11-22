# Compile universal-node protocol + Trezor crypto for native (host) tests.
Import("env")

from pathlib import Path

project = Path(env["PROJECT_DIR"]).resolve()
src = project.parents[1] / "src"

env.Append(
    CPPPATH=[str(src), str(src / "Trezor")],
    CCFLAGS=["-ffunction-sections", "-fdata-sections"],
    LINKFLAGS=["-Wl,--gc-sections"],
)

env.BuildSources(
    str(Path(env["BUILD_DIR"]) / "web3e_protocol"),
    str(src),
    [
        "+<UniversalNodeProtocol.cpp>",
        "+<Trezor/sha3.c>",
        "+<Trezor/ecdsa.c>",
        "+<Trezor/bignum.c>",
        "+<Trezor/secp256k1.c>",
        "+<Trezor/memzero.c>",
        "+<Trezor/hmac.c>",
        "+<Trezor/rfc6979.c>",
        "+<Trezor/sha2.c>",
    ],
)

# Arduino-free stubs for symbols referenced by Trezor rand/hmac paths
env.BuildSources(
    str(Path(env["BUILD_DIR"]) / "native_stubs"),
    str(project),
    ["+<native_stubs.c>"],
)
