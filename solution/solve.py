from request_pb2 import *
import sys
import socket
import ssl
import struct
import random

s = socket.create_connection((sys.argv[1], 2525))
s = ssl.wrap_socket(s)

def readall(s, length):
    result = ""
    while len(result) < length:
        result += s.read(length - len(result))
    return result

def send_request(s, t, r = None):
    if r is None:
        data = ""
    else:
        data = r.SerializeToString()
    req = Request()
    req.type = t
    req.data = data
    data = req.SerializeToString()
    s.write(struct.pack("<H", len(data)) + data)

def get_response(s):
    length = struct.unpack("<I", readall(s, 4))[0]
    return readall(s, length)

def register(s, name, password):
    req = RegisterRequest()
    req.username = name
    req.password = password
    send_request(s, Request.Register, req)

    resp = RegisterResponse()
    resp.ParseFromString(get_response(s))
    return resp.status

def get_monsters_in_range(s, x, y):
    req = GetMonstersInRangeRequest()
    req.x = x
    req.y = y
    send_request(s, Request.GetMonstersInRange, req)
    resp = GetMonstersInRangeResponse()
    resp.ParseFromString(get_response(s))
    result = []
    for sighting in resp.sightings:
        result.append((sighting.x, sighting.y))
    return result

username = hex(random.randint(0, 2 ** 32))[2:]
password = hex(random.randint(0, 2 ** 32))[2:]

print "Username: " + username
print "Password: " + password

register(s, username, password)

stops = []
for x in xrange(-2048, 2048, 128):
    for y in xrange(-2048, 2048, 128):
        req = GetMapTilesRequest()
        req.x = x
        req.y = y
        send_request(s, Request.GetMapTiles, req)
        resp = GetMapTilesResponse()
        resp.ParseFromString(get_response(s))
        data = resp.data
        for i in xrange(0, 128 * 128 / 2):
            ch = ord(data[i])
            if (ch & 0xf) == 0xf:
                stops.append((x + ((i % 64) * 2), y + (i / 64)))
            if ((ch >> 4) & 0xf) == 0xf:
                stops.append((x + 1 + ((i % 64) * 2), y + (i / 64)))

send_request(s, Request.GetPlayerDetails)
resp = GetPlayerDetailsResponse()
resp.ParseFromString(get_response(s))
x = resp.x
y = resp.y

stop_id = random.randint(0, len(stops) - 1)
last_level = 0

while True:
    send_request(s, Request.GetPlayerDetails)
    resp = GetPlayerDetailsResponse()
    resp.ParseFromString(get_response(s))
    level = resp.level

    if last_level != level:
        print "Level %d" % level
        last_level = level

    send_request(s, Request.GetInventory)
    resp = GetInventoryResponse()
    resp.ParseFromString(get_response(s))
    balls = 0
    for i in resp.items:
        if i.item == 0:
            balls = i.count

    if balls < 50:
        req = GetItemsFromStopRequest()
        req.x = stops[stop_id][0]
        req.y = stops[stop_id][1]
        stop_id += 1
        send_request(s, Request.GetItemsFromStop, req)
        resp = GetItemsFromStopResponse()
        resp.ParseFromString(get_response(s))
        continue

    monster = get_monsters_in_range(s, x, y)
    if len(monster) == 0:
        x += random.randint(0, 512) - 128
        y += random.randint(0, 512) - 128
        if x < -2048:
            x += 1024
        if x > 2048:
            x -= 1024
        if y < -2048:
            y += 1024
        if y > 2048:
            y -= 1024
        continue

    req = StartEncounterRequest()
    req.x = monster[0][0]
    req.y = monster[0][1]
    send_request(s, Request.StartEncounter, req)
    resp = StartEncounterResponse()
    resp.ParseFromString(get_response(s))
    if not resp.valid:
        print "Invalid encounter"
        break

    while True:
        req = ThrowBallRequest()
        req.ball = 0
        send_request(s, Request.ThrowBall, req)
        resp = ThrowBallResponse()
        resp.ParseFromString(get_response(s))
        if resp.result == ThrowBallResponse.THROW_RESULT_CATCH:
            req = EvolveMonsterRequest()
            req.id = resp.catchid
            send_request(s, Request.EvolveMonster, req)
            resp = EvolveMonsterResponse()
            resp.ParseFromString(get_response(s))
            break
        if resp.result == ThrowBallResponse.THROW_RESULT_RUN_AWAY_AFTER_ONE:
            break
        if resp.result == ThrowBallResponse.THROW_RESULT_RUN_AWAY_AFTER_TWO:
            break
        if resp.result == ThrowBallResponse.THROW_RESULT_RUN_AWAY_AFTER_THREE:
            break
