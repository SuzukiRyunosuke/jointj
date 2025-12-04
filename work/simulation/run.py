import sys
import os
import subprocess
import shutil
from pathlib import Path

s = subprocess
if "POLYFEM_ROOTDIR" not in os.environ:
    os.environ["POLYFEM_ROOTDIR"] = \
            s.run(["git", "rev-parse", "--show-toplevel"], check=True, stdout=s.PIPE).stdout.decode("utf-8").rstrip()
root_dir = os.environ["POLYFEM_ROOTDIR"]
build_dir = root_dir + "/build/"
run_dir = root_dir + '/work/simulation/'
script_dir = run_dir + 'scripts/'

MESH_DIR = os.path.join(run_dir, "out")

BASES = ["concave", "convex"]

ZERO_PAD = False

def _iter_str(i: int) -> str:
    return f"{i:03d}" if ZERO_PAD else str(i)

def mesh_path(base: str, iter_no: int) -> Path:
    return Path(MESH_DIR) / f"{base}_iter_{_iter_str(iter_no)}.msh"

def ensure_meshes_for_next_iter(prev_iter: int, next_iter: int):
    for base in BASES:
        src = mesh_path(base, prev_iter)
        dst = mesh_path(base, next_iter)
        if dst.exists():
            continue
        if not src.exists():
            raise FileNotFoundError(
                f"[ensure_meshes_for_next_iter] missing src: {src}（{base}, prev_iter={prev_iter}）"
            )
        shutil.copy(src, dst)
        print(f"[info] remesh skipped → copied {src.name} → {dst.name}")

if len(sys.argv) < 2:
    iteration = int(input("initial iteration: "))
    bootstrap = iteration
else:
    iteration = 0
    bootstrap = int(sys.argv[1])
continue_till = bootstrap
run_type = "parallel"
inc = 20
flag = True
sh = ['bash']
while flag:
    overwrite_sh = sh + [script_dir + 'overwrite_json.sh', str(iteration)]
    s.run(overwrite_sh, check=True)
    build_sh = sh + [build_dir + 'compile.sh']
    run_sh = sh + [script_dir + 'run.sh', run_type]
    computation_success = False
    try:
        if os.path.exists(build_dir + 'compile.sh'):
            s.run(build_sh, check=True)
        s.run(run_sh, check=True)
        computation_success = True
        print(f"calculation from iter:{iteration} to iter:{iteration+inc} has done.")
    except:
        inc = 20
        continue_till = iteration
        print("calculation failed.")

    if computation_success and iteration + inc < continue_till:
        backup_sh = sh + [script_dir + 'backup.sh', str(iteration)]
        s.run(backup_sh)
        iteration += inc
        remesh_sh = sh + [script_dir + 'remesh.sh', str(iteration)]
        s.run(remesh_sh)
        continue
    res = "n"
    if computation_success:
        res = input("proceed to next? (Y/n): ")
    if res == "n":
        res = input("go back? if so input the iteration number: ")
        try:
            iteration = int(res)
            print(f"go back to iter: {iteration}")
        except:
            print(f"retry iter: {iteration}")
        pull_backup_sh = sh + [script_dir + 'pull_backup.sh', str(iteration)]
        s.run(pull_backup_sh)
    else:
        backup_sh = sh + [script_dir + 'backup.sh', str(iteration)]
        s.run(backup_sh)
        iteration += inc
        # go foward
        try:
            continue_till = bootstrap
            forward_by = int(res)
            continue_till = iteration + forward_by
            print(f"will go further forward by: {forward_by}")
        except:
            print(f"will go by {inc}")

    if input("remesh? (Y/n): ") == "n":
        try:
            ensure_meshes_for_next_iter(iteration - inc, iteration)
        except Exception as e:
            print(f"[error] failed to prepare meshes for iter:{iteration} after skip → {e}")
        print(f"remeshing at iter:{iteration} was skipped.")
    else:
        remesh_sh = sh + [script_dir + 'remesh.sh', str(iteration)]
        s.run(remesh_sh)
        print(f"system at iter: {iteration} was remeshed.")
