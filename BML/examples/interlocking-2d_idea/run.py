import sys
import os
import subprocess
s = subprocess
if "POLYFEM_ROOTDIR" not in os.environ:
    os.environ["POLYFEM_ROOTDIR"] = \
            s.run(["git", "rev-parse", "--show-toplevel"], check=True, stdout=s.PIPE).stdout.decode("utf-8").rstrip()
root_dir = os.environ["POLYFEM_ROOTDIR"]
build_dir = root_dir + "/build/"
run_dir = root_dir + '/BML/examples/interlocking-2d_idea/'
script_dir = run_dir + 'scripts/'
if len(sys.argv) < 2:
    iteration = int(input("initial iteration: "))
    bootstrap = iteration
else:
    iteration = 0
    bootstrap = int(sys.argv[1])
continue_till = bootstrap
run_type = "parallel"
inc = 10 # initially we go from 0 to inc
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
        inc = 10
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
        print(f"remeshing at iter:{iteration} was skipped.")
    else:
        remesh_sh = sh + [script_dir + 'remesh.sh', str(iteration)]
        s.run(remesh_sh)
        print(f"system at iter: {iteration} was remeshed.")
