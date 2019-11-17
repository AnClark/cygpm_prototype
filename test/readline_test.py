import re

s = open("setup.ini", "rb").read().decode("utf-8")

res = re.findall(".*\n", s, re.M)

for i in res:
    print(i)

print(len(res))