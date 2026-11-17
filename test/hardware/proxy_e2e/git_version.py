Import("env")
import subprocess

try:
    # Get short git SHA
    git_sha = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd='../../..').decode('ascii').strip()
    fw_version = 'e2e-{}'.format(git_sha)
    env.Append(CPPDEFINES=[('FW_VERSION', '\\"{}\\"'.format(fw_version))])
    print('Building with firmware version: {}'.format(fw_version))
except Exception as e:
    print('Warning: Could not get git SHA: {}'.format(e))
    print('Using default FW_VERSION from platformio.ini')
