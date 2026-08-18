print("const static uint8_t pattern[] PROGMEM = {")
data = []
r=2+(30*16)
for i in range(r):
  data.append(0)
data.append('')
data[0]=8 #duration
j = 2
s=","
for i in range(20):
  data[1]=i # sequencenumber
  data[j]=9
  print(s.join(str(x) for x in data))
  data[j]=1
  j= j+1
print("};")
print("const static uint8_t restpattern[] PROGMEM = {};")
