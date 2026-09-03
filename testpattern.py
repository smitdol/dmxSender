print("const static uint8_t pattern[] PROGMEM = {")
data = []
r=3+(30*16)
for i in range(1,r):
  data.append(0)
data.append('')
data[0]=4 #duration
j = 2
s=","
for i in range(1,68):
  data[1]=i # sequencenumber
  data[j]=2
  print(s.join(str(x) for x in data))
  #data[j]=0
  j= j+1
print("};")
print("const static uint8_t restpattern[] PROGMEM = {};")
