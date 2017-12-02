import string
import random

f1=open("file1.txt","w+")
f2=open("file2.txt", "w+")
f3=open("file3.txt", "w+")

for i in range(10):
    f1.write(random.choice(string.ascii_lowercase))
    f2.write(random.choice(string.ascii_lowercase))
    f3.write(random.choice(string.ascii_lowercase))
f1.close()
f2.close()
f3.close()
#open to read files
f1=open("file1.txt","r")
f2=open("file2.txt", "r")
f3=open("file3.txt", "r")

if f1.mode == 'r':
    contents = f1.read()
    print(contents)
if f2.mode == 'r':
    contents = f2.read()
    print(contents)
if f3.mode == 'r':
    contents = f3.read()
    print(contents)
random.seed(1)
num1 = random.randint(1,43)
random.seed(2)
num2 = random.randint(1,43)
print(num1)
print(num2)
print(num1*num2)
