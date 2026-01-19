import os, sys, subprocess, time

rnic_name = "mlx5_0"
rnic_port = 1
nat_ip = "10.10.1.1"
count_clients = 2

count_hugepages = 128  # INSERT_COUNT == 2 ^ 26 <==> size_data == 256 MB <==> 128 hugepages(2 MB)

path_project = os.sep.join(["D:", "study", "rdma-filter"])
path_list_machines = os.sep.join([path_project, "scripts", "list_machines.txt"])
path_temp = os.sep.join([path_project, "scripts", "temp"])
path_output = os.sep.join([path_project, "output"])
path_script_log = os.sep.join([path_project, "scripts", "script.log"])

path_ssh_local = os.sep.join(["C:", "Users", "Yuandu", ".ssh"])
path_public_key_cloudlab = os.sep.join([path_ssh_local, "cloudlab.pub"])
path_private_key_cloudlab = os.sep.join([path_ssh_local, "cloudlab"])
path_private_key_rsa = os.sep.join([path_ssh_local, "id_rsa"])

class Command:
    words: list

    def __init__(self, words: list):
        self.words = [str(word) for word in words]
    
    def __str__(self):
        return " ".join(self.words)

class CommandList:
    commands: list

    def __init__(self, commands: list):
        self.commands = commands
    
    def __str__(self):
        return "; ".join([str(command) for command in self.commands])
    
    def __and__(self, other):
        return CommandList(self.commands + other.commands)

def log(label: str = None, msg: str = "", level: str = "INFO"):
    with open(path_script_log, "a") as log_file:
        log_file.write(f"[{level}] [{time.strftime('%Y-%m-%d %H:%M:%S')}] {f"[{label}]" if label != None else ''} {msg}\n")

def scp_send(remote_machine : str, list_local_path : list, remote_path : str):
    cmd = ["scp", "-o", "StrictHostKeyChecking=no"] + list_local_path + [f"{remote_machine}:{remote_path}"]
    if len(list_local_path) > 1:
        cmd.insert(1, "-r")
    log( "scp_send", f"Execute: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True, stderr=sys.stderr)
    except subprocess.CalledProcessError as e:
        log("scp_send", f"failed: {e}")

def scp_recv_single(remote_machine : str, remote_path : str, local_path : str):
    cmd = ["scp", "-o", "StrictHostKeyChecking=no", f"{remote_machine}:{remote_path}", local_path]
    log("scp_recv_single", f"Execute: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True, stderr=sys.stderr)
    except subprocess.CalledProcessError as e:
        log("scp_recv_single", f"failed: {e}")

def cmd_tmux_kill(session_id : str):
    return Command(["tmux", "kill-session", "-t", session_id, "2>/dev/null", "||", "true"])

def cmd_tmux_new(session_id : str, list_cmd : CommandList):
    return Command(["tmux", "new-session", "-d", "-s", session_id, f"'{str(list_cmd)}'"])

def ssh_exec(remote_machine : str, list_cmd : CommandList, use_tmux: bool = False, tmux_session_id: str = "test"):
    cmd = ["ssh", "-o", "StrictHostKeyChecking=no", "-i", path_private_key_rsa, remote_machine]
    str_cmd = None
    if use_tmux:
        str_cmd = str(CommandList([
            cmd_tmux_kill(tmux_session_id),
            cmd_tmux_new(tmux_session_id, list_cmd)
        ]))
    else:
        str_cmd = str(list_cmd)
    cmd.append(f"{str_cmd}")
    log("ssh_exec", f"Execute: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        log("ssh_exec", f"failed: {e}")
    finally:
        pass
        # print(result.stdout.decode())
        # print(result.stderr.decode())

def temp():
    pass

if __name__ == "__main__":
    little_tip = "Valid argument: init, compile, deploy, run, collect, showinfo, perftest"
    if len(sys.argv) < 2:
        print(little_tip)
        sys.exit(1)
    
    list_machines = (open(path_list_machines).read().splitlines())[0 : count_clients + 1]
    log("main", f"{len(list_machines)} Machines: {list_machines}")

    if sys.argv[1] == "init":
        list_cmd_init = CommandList([
            Command(["mkdir", "exp1", ">", "init.log", "2>&1"]), 
            Command(["mkdir", "exp1/build", ">>", "init.log", "2>&1"]),
            Command(["sudo", "apt", "update", ">>", "init.log", "2>&1"]), 
            Command(["sudo", "apt", "install", "-y", "cmake", "libibverbs-dev", "rdma-core", "librdmacm1", "librdmacm-dev",     "ibverbs-utils", "infiniband-diags", "perftest", "linux-tools-common", "linux-tools-generic", "linux-cloud-tools-generic", "tmux", ">>", "init.log", "2>&1"])
        ])
        list_cmd_chmod = CommandList([
            Command(["chmod", "700", ".ssh"]),
            Command(["chmod", "600", ".ssh/cloudlab"]),
        ])
        for machine in list_machines:
            pass
            ssh_exec(machine, list_cmd_init)
        with open(path_public_key_cloudlab) as f:
            pub_key_content = f.read()
        for machine in list_machines[1:]:
            scp_recv_single(machine, ".ssh/authorized_keys", path_temp)
            try:
                with open(os.sep.join([path_temp, "authorized_keys"]), "r") as f:
                    existing_keys = set(line.strip() for line in f if line.strip() and not line.startswith("#"))
            except FileNotFoundError:
                log("init", f"No authorized_keys file on {machine}", level="WARNING")
                break
            else:
                if (pub_key_content.strip() not in existing_keys):
                    log("init", f"Adding public key to {machine}'s authorized_keys")
                    with open(os.sep.join([path_temp, "authorized_keys"]), "a") as f:
                        f.write(pub_key_content.strip() + "\n")
                    scp_send(machine, [os.sep.join([path_temp, "authorized_keys"])], ".ssh")
        scp_send(list_machines[0], [path_private_key_cloudlab], ".ssh")
        ssh_exec(list_machines[0], list_cmd_chmod)
    
    elif sys.argv[1] == "showinfo":
        list_cmd_show_info = CommandList([
            Command(["ip", "addr"]),
            Command(["ibstat"]),
            Command(["ibv_devinfo"])
        ])
        ssh_exec(list_machines[0], list_cmd_show_info)
    
    elif sys.argv[1] == "compile":
        list_cmake = ["cmake", "..", ">", "compile.log", "2>&1"]
        for i in range(2, len(sys.argv)):
            list_cmake.insert(i - 1, str(sys.argv[i]))
        list_cmd_compile = CommandList([
            Command(["cd", "exp1/build"]),
            Command(["rm", "-rf", "*"]),
            Command(list_cmake),
            Command(["make", ">>", "compile.log", "2>&1"])
        ])
        scp_send(list_machines[0], [
            os.sep.join([path_project, "src"]), 
            os.sep.join([path_project, "test"]), 
            os.sep.join([path_project, "CMakeLists.txt"])], "exp1")
        ssh_exec(list_machines[0], list_cmd_compile)
        scp_recv_single(list_machines[0], "exp1/build/compile.log", os.sep.join([path_output, "compile.log"]))

    elif sys.argv[1] == "deploy":
        for machine in list_machines[1:]:
            ssh_exec(list_machines[0], CommandList([
                Command(["scp", "-r", "-o", "StrictHostKeyChecking=no", "-i", ".ssh/cloudlab", "exp1/build", f"{machine}:exp1/"]),
            ]))

    elif sys.argv[1] == "run":
        cmd_server_run = Command(["./exp1/build/test/2_srv", ">", "out.log", "2>&1"])
        cmd_client_run = Command(["./exp1/build/test/2_cli", ">", "out.log", "2>&1"])
        ssh_exec(list_machines[0], CommandList([cmd_server_run]), use_tmux=True, tmux_session_id="exp")
        for machine in list_machines[1:]:
            ssh_exec(machine, CommandList([cmd_client_run]), use_tmux=True, tmux_session_id="exp")
        print(f"Running exp. Current Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    elif sys.argv[1] == "collect":
        for i in range(len(list_machines)):
            try:
                scp_recv_single(list_machines[i], "out.log", os.sep.join([path_output, f"out_{i}.log"]))
            except subprocess.CalledProcessError as e:
                print(f"Received machine {i} failed: {e}")
            else:
                print(f"Received machine {i}.")
        print(f"Collect completed. Current Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    elif sys.argv[1] == "stop":
        for machine in list_machines:
            ssh_exec(machine, CommandList([cmd_tmux_kill("exp")]))
        log("stop", "All experiment tmux sessions killed.")
    
    elif sys.argv[1] == "perftest":
        list_cmd_perftest_server = CommandList([
            Command(["ib_send_bw", "-d", rnic_name, "-i", str(rnic_port), ">", "test.log", "2>&1"])
        ])
        list_cmd_perftest_client = CommandList([
            Command(["ib_send_bw", "-d", rnic_name, "-i", str(rnic_port), nat_ip, ">", "test.log", "2>&1"])
        ])
        ssh_exec(list_machines[0], list_cmd_perftest_server, use_tmux=True, tmux_session_id="perftest")
        ssh_exec(list_machines[1], list_cmd_perftest_client, use_tmux=True, tmux_session_id="perftest")
        time.sleep(5)
        scp_recv_single(list_machines[0], "test.log", os.sep.join([path_output, "perftest_server.log"]))
        scp_recv_single(list_machines[1], "test.log", os.sep.join([path_output, "perftest_client.log"]))

    elif sys.argv[1] == "clear":
        list_cmd_user_clear = CommandList([
            Command(["rm", "-rf", "exp1"]),
            Command(["rm", "-f", "*.flag"]),
            Command(["rm", "-f", "*.log"])
        ])
        for machine in list_machines:
            ssh_exec(machine, list_cmd_user_clear)

    elif sys.argv[1] == "init_hugepage":
        list_cmd_hugepages = CommandList([
            Command(["bash", "-c", f"'echo {count_hugepages} | sudo tee /proc/sys/vm/nr_hugepages'"]),
            Command(["grep", "Huge", "/proc/meminfo"]),
            Command(["sudo", "mkdir", "-p", "/mnt/huge"]),
            Command(["sudo", "umount", "/mnt/huge", "2>/dev/null", "||", "true"]),  # 先卸载，忽略错误
            Command(["sudo", "mount", "-t", "hugetlbfs", "-o", "pagesize=2M", "none", "/mnt/huge"]),
            Command(["sudo", "chmod", "777", "/mnt/huge"]),  # 给予写权限
            Command(["mount", "|", "grep", "huge"]),  # 验证挂载
        ])
        ssh_exec(list_machines[0], list_cmd_hugepages)

    elif sys.argv[1] == "test":
        pass
        list_cmd_chmod = CommandList([
            Command(["chmod", "700", ".ssh"]),
            Command(["chmod", "600", ".ssh/cloudlab"]),
        ])
        ssh_exec(list_machines[0], list_cmd_chmod)
    
    else:
        print(little_tip)