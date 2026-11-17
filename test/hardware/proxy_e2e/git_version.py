Import("env")
import subprocess
import os

try:
    # Get short git SHA from the Web3E repo root
    repo_root = os.path.join(env['PROJECT_DIR'], '../../..')
    git_sha = subprocess.check_output(
        ['git', 'rev-parse', '--short', 'HEAD'], 
        cwd=repo_root
    ).decode('ascii').strip()
    
    fw_version = 'e2e-{}'.format(git_sha)
    
    # Remove any existing FW_VERSION definitions
    env['CPPDEFINES'] = [
        define for define in env['CPPDEFINES'] 
        if not (isinstance(define, tuple) and define[0] == 'FW_VERSION')
        and not (isinstance(define, str) and define.startswith('FW_VERSION='))
    ]
    
    # Add our FW_VERSION
    env.Append(CPPDEFINES=[('FW_VERSION', '\\"{}\\"'.format(fw_version))])
    print('Building with firmware version: {}'.format(fw_version))
    
except Exception as e:
    print('Warning: Could not get git SHA: {}'.format(e))
    # Fallback to a default
    env.Append(CPPDEFINES=[('FW_VERSION', '\\"e2e-unknown\\"')])
    print('Using fallback firmware version: e2e-unknown')
