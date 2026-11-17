Import("env")
import subprocess
import os

def get_git_version():
    """Get e2e-<short_sha>[-dirty] version string, or 'e2e-unknown' if git unavailable."""
    try:
        # Get absolute path to Web3E repo root (3 levels up from this script)
        # SCons exec context: use env['PROJECT_DIR'] if available
        try:
            project_dir = env['PROJECT_DIR']
        except:
            # Fallback: assume this script is in test/hardware/proxy_e2e/
            project_dir = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else os.getcwd()
        
        repo_root = os.path.abspath(os.path.join(project_dir, '..', '..', '..'))
        
        # Check if git is available and this is a git repo
        try:
            subprocess.check_output(
                ['git', 'rev-parse', '--git-dir'],
                cwd=repo_root,
                stderr=subprocess.STDOUT
            )
        except (subprocess.CalledProcessError, OSError):
            return 'e2e-unknown'
        
        # Get short git SHA
        git_sha = subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD'],
            cwd=repo_root,
            stderr=subprocess.STDOUT
        ).decode('ascii').strip()
        
        # Check if working tree is dirty (ignore untracked files to avoid build artifacts)
        try:
            # Use git diff to check tracked files only
            subprocess.check_output(
                ['git', 'diff', '--quiet', 'HEAD'],
                cwd=repo_root,
                stderr=subprocess.STDOUT
            )
            is_dirty = False
        except subprocess.CalledProcessError:
            # Exit code 1 means there are differences
            is_dirty = True
        except:
            is_dirty = False
        
        # Build version string (max 32 ASCII chars)
        fw_version = 'e2e-{}{}'.format(git_sha, '-dirty' if is_dirty else '')
        return fw_version[:32]  # Ensure <=32 chars
        
    except Exception as e:
        print('Warning: Could not get git version: {}'.format(e))
        return 'e2e-unknown'

fw_version = get_git_version()

# Initialize CPPDEFINES if not present
if 'CPPDEFINES' not in env:
    env['CPPDEFINES'] = []

# Remove any existing FW_VERSION definitions
env['CPPDEFINES'] = [
    define for define in env.get('CPPDEFINES', [])
    if not (isinstance(define, tuple) and define[0] == 'FW_VERSION')
    and not (isinstance(define, str) and define.startswith('FW_VERSION='))
]

# Add our FW_VERSION
env.Append(CPPDEFINES=[('FW_VERSION', '\\"{}\\"'.format(fw_version))])
print('Building with firmware version: {}'.format(fw_version))
