import subprocess
import shutil
import os

map_str = input("输入地图编号(1-8，默认1)：")
map = int(map_str) if len(map_str) > 0 else 1
if map not in range(1, 8+1):
    map = 1

# print("Compiling program...")
subprocess.run("g++ main.cpp -o main.exe", shell=True)

# print("Compiling program(input_test)...")
# subprocess.run("g++ test_input.cpp -o main.exe", shell=True)

# print("Moving file...")
try:
    os.remove("./WindowsRelease/main.exe")
except:
    pass

shutil.move("./main.exe", "./WindowsRelease")
os.chdir("./WindowsRelease")

# print("Running test...")
# Custom
subprocess.run(
        # r"robot_gui.exe .\main.exe -m .\maps\1.txt -l DBG",
        f"robot_gui.exe .\\main.exe -m .\\maps\\{map}.txt",
        shell=True)
# Demo
# subprocess.run(
#         r"robot_gui.exe .\Demo\SimpleDemo.exe -m .\maps\1.txt",
#         shell=True)
