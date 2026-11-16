# Releasing Web3E

This document describes the process for releasing a new version of Web3E to the PlatformIO Registry.

## Release Process

1. **Update Version**
   - Increment the `version` field in `library.json` (e.g., from `1.45` to `1.46`)
   - Increment the `version` field in `library.properties` to match
   - Commit the version bump to a PR and get it reviewed

2. **Merge to Master**
   - Merge the PR with the version bump into `master`
   - Ensure all CI checks pass

3. **Create GitHub Release**
   - Go to [GitHub Releases](https://github.com/AlphaWallet/Web3E/releases)
   - Click "Draft a new release"
   - Create a new tag matching the version with a `v` prefix (e.g., `v1.46` for version `1.46`)
   - Set the target to `master`
   - Write release notes describing the changes
   - Click "Publish release"

4. **Automated Publishing**
   - The `release.yml` GitHub Actions workflow will automatically:
     - Verify the tag matches the `library.json` version
     - Run offline tests to ensure the library compiles
     - Run native tests (HTTP parser)
     - Pack the library (excluding tests, tools, and dev files)
     - Publish to the [PlatformIO Registry](https://registry.platformio.org/libraries/alphawallet/Web3E)
     - Attach the packed library tarball to the GitHub Release

5. **Verify Release**
   - Check the [Actions tab](https://github.com/AlphaWallet/Web3E/actions) to ensure the workflow completed successfully
   - Verify the new version appears on the [PlatformIO Registry](https://registry.platformio.org/libraries/alphawallet/Web3E)
   - Check that the tarball was attached to the GitHub Release

## Testing the Release Workflow

You can test the release workflow without publishing by using the manual workflow dispatch:

1. Go to [Actions → Release to PlatformIO Registry](https://github.com/AlphaWallet/Web3E/actions/workflows/release.yml)
2. Click "Run workflow"
3. Keep "Dry run" checked (default: true)
4. Select a branch/tag
5. Click "Run workflow"

In dry run mode, the workflow will:
- Verify the version
- Run all tests
- Pack the library
- Upload the tarball as a workflow artifact (instead of publishing)

## Version Mismatch

If the tag version doesn't match the `library.json` version, the workflow will fail with a clear error message. For example:

- ✅ Tag `v1.45` with `library.json` version `1.45` → Success
- ❌ Tag `v1.46` with `library.json` version `1.45` → Fail

Always ensure the version in `library.json` is bumped and merged to master before creating the release.

## What Gets Published

The library package includes:
- `src/` - Source code
- `examples/` - Example projects
- `library.json` - Library metadata
- `library.properties` - Arduino library metadata

The following are excluded from the package (see `library.json` → `export.exclude`):
- `tests/` - Test suite
- `test-consumer/` - Test consumer project
- `tools/` - Development tools
- `.github/` - GitHub workflows
- `docs/` - Documentation
- Build artifacts and caches

## Requirements

The workflow requires the `PLATFORMIO_AUTH_TOKEN` secret to be set in the repository. This should be configured by the repository administrator.
