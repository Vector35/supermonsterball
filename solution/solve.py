from request_pb2 import *
import sys
import socket
import ssl
import struct
import random
import time

s = socket.create_connection((sys.argv[1], 2525))
s = ssl.wrap_socket(s)

ignored_response_count = 0
challenge = None

def readall(s, length):
    result = ""
    while len(result) < length:
        data = s.read(length - len(result))
        if len(data) == 0:
            raise Exception, "Disconnected"
        result += data
    return result

def send_request(s, t, r = None):
    if r is None:
        data = ""
    elif type(r) == str:
        data = r
    else:
        data = r.SerializeToString()
    req = Request()
    req.type = t
    req.data = data
    data = req.SerializeToString()
    s.write(struct.pack("<H", len(data)) + data)

def get_response(s):
    global ignored_response_count
    for i in xrange(0, ignored_response_count):
        length = struct.unpack("<I", readall(s, 4))[0]
        readall(s, length)
    ignored_response_count = 0
    length = struct.unpack("<I", readall(s, 4))[0]
    return readall(s, length)

def ignore_response():
    global ignored_response_count
    ignored_response_count += 1

def register(s, name, password):
    global challenge
    req = RegisterRequest()
    req.username = name
    req.password = password
    send_request(s, Request.Register, req)

    resp = RegisterResponse()
    resp.ParseFromString(get_response(s))
    challenge = resp.connectionid
    return resp.status

def get_challenge_response(value):
    value ^= 0xc0decafefeedface
    mix = ((value * 25214903917) + 11) & 0xffffffffffffffff;
    value = ((value >> 17) | (value << 47)) & 0xffffffffffffffff;
    value = (value + mix) & 0xffffffffffffffff;
    return value

def get_encounter_validation(x, y):
    seed = (((x & 0xffffffffffffffff) * 694847539) + ((y & 0xffffffffffffffff) * 91939)) + 92893
    return (seed >> 16) & 0xffffffff

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

def report_location(s, x, y):
    req = GetMonstersInRangeRequest()
    req.x = x
    req.y = y
    send_request(s, Request.GetMonstersInRange, req)
    ignore_response()

def step_move(s, x, y):
    global last_x, last_y
    while abs(last_x - x) > 50:
        if last_x < x:
            last_x += 50
        else:
            last_x -= 50
        report_location(s, last_x, last_y)
    while abs(last_y - y) > 50:
        if last_y < y:
            last_y += 50
        else:
            last_y -= 50
        report_location(s, last_x, last_y)

    last_x = x
    last_y = y

username = hex(random.randint(0, 2 ** 32))[2:]
password = hex(random.randint(0, 2 ** 32))[2:]

print "Username: " + username
print "Password: " + password

register(s, username, password)

# Read the map data to find all the stops so that we can get more balls
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

# One GetAllPlayerInfo request must be made with an 8 byte challenge/reponse token before performing any
# throws or gathing any items, otherwise a ban
send_request(s, Request.GetAllPlayerInfo, struct.pack("<Q", get_challenge_response(challenge)))
resp = GetAllPlayerInfoResponse()
resp.ParseFromString(get_response(s))
x = resp.player.x
y = resp.player.y
last_x = x
last_y = y
x_move = 64

stop_id = random.randint(0, len(stops) - 1)
last_level = 0

stop_round_time = time.time()
catch_round_time = time.time()

start_time = time.time()

while True:
    # Check current level
    send_request(s, Request.GetPlayerDetails)
    resp = GetPlayerDetailsResponse()
    resp.ParseFromString(get_response(s))
    level = resp.level
    xp = resp.xp

    if last_level != level:
        print "Level %d (%d XP)" % (level, xp)
        last_level = level
        if level == 40:
            break

    # Get current inventory to get ball counts
    send_request(s, Request.GetInventory)
    resp = GetInventoryResponse()
    resp.ParseFromString(get_response(s))
    balls = 0
    super_balls = 0
    uber_balls = 0
    for i in resp.items:
        if i.item == 0:
            balls = i.count
        if i.item == 1:
            super_balls = i.count
        if i.item == 2:
            uber_balls = i.count

    if balls < 10:
        # Low on balls, get some more
        req = GetItemsFromStopRequest()
        req.x = stops[stop_id][0]
        req.y = stops[stop_id][1]

        # Don't move too far to avoid spoof ban
        step_move(s, req.x, req.y)

        stop_id += 1
        if stop_id >= len(stops):
            # Visited all the stops in the map, if there hasn't been enough time for refresh, wait
            to_wait = int(60 - (time.time() - stop_round_time))
            if to_wait > 0:
                print "Used up all stops, waiting for %d seconds" % to_wait
                time.sleep(to_wait)
            stop_round_time = time.time()
            stop_id = 0

        send_request(s, Request.GetItemsFromStop, req)
        resp = GetItemsFromStopResponse()
        resp.ParseFromString(get_response(s))
        continue

    # Don't move too far to avoid spoof ban
    step_move(s, x, y)

    # Look for nearby monsters
    monster = get_monsters_in_range(s, x, y)
    if len(monster) == 0:
        # Nothing nearby, move 64 units and try again, covering the entire map
        x += x_move
        if x >= 2048:
            x = 2048 - 64
            y += 64
            x_move = -64
        elif x <= -2048:
            x = (-2048) + 64
            y += 64
            x_move = 64
        if y > 2048:
            y = (-2048) + 64
        if (x == 0) and (y == 0):
            to_wait = int(600 - (time.time() - catch_round_time))
            if to_wait > 0:
                # Entire map explored, if the time spent is not enough for new spawns, wait
                print "Explored entire map, waiting %d seconds for respawns" % to_wait
                time.sleep(to_wait)
            catch_round_time = time.time()
        continue

    # Try to capture anything in range
    for m in monster:
        # Start the encounter for the current monster
        req = StartEncounterRequest()
        req.x = m[0]
        req.y = m[1]
        req.data = get_encounter_validation(req.x, req.y)
        step_move(s, req.x, req.y)
        send_request(s, Request.StartEncounter, req)
        resp = StartEncounterResponse()
        resp.ParseFromString(get_response(s))
        if not resp.valid:
            break

        # Throw balls at it until it's captured
        throw_count = 0
        while True:
            req = ThrowBallRequest()
            if uber_balls > 0:
                req.ball = 2
                uber_balls -= 1
            elif super_balls > 0:
                req.ball = 1
                super_balls -= 1
            else:
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
            throw_count += 1
            if throw_count > 5:
                send_request(s, Request.RunFromEncounter)
                ignore_response()

print "Reaching level 40 took %d seconds" % int(time.time() - start_time)

# Grab the flag
send_request(s, Request.GetLevel40Flag)
resp = GetLevel40FlagResponse()
resp.ParseFromString(get_response(s))
print resp.flag
