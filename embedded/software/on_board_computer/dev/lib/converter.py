import base64


f = open("image.txt", "r")

image_str = f.read()
decoded = base64.b64decode(image_str)

s = open("image.jpg", "wb")
#print(bytes.fromhex(image_str))
s.write(decoded)
s.close()
f.close()