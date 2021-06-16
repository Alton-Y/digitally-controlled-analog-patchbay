import websocket
ws = websocket.WebSocket()
ws.connect("ws://10.10.69.139/ws")
while True:
  result = ws.recv()
  print(result)

