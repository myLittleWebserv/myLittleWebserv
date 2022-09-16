import os
import string
import random

#print(os.environ)

def get_session_id() :
  string_pool = string.ascii_lowercase + string.digits

  result = ""
  for i in range(16):
    result += random.choice(string_pool)
  return result

def main():
  os.makedirs("database", exist_ok=True)
  print("Status: 200 OK\r")
  print("Content-Type: text/html; charset=utf-8\r")
  try:
    cookies = os.environ['HTTP-COOKIE'].split(';')[0]
    session_dict = cookies.split('=')
    if session_dict[0] != 'tsession':
      raise Exception()
    session_id = session_dict[1]
    f = open("database/" + session_id, 'r')
    count = int(f.read())
    count += 1
    f.close()
    f = open("database/" + session_id, 'w')
    f.write(str(count))
    f.close()
    print("\r")
    print(count, end='')
  except:
    session_id = get_session_id()
    print("Set-Cookie: tsession=" + session_id + "\r\n\r")
    f = open("database/" + session_id, 'w')
    f.write("1")
    f.close()
    print("1", end='')

if __name__ == "__main__":
  main()