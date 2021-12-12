# Funktion: Daten über einen Websocket-Server eines Mikrocontrollers auslesen diese in
#           einer MySQL-Datenbank speichern.
#           Einen Telegram-Bot steuern
#
# Autoren: Lisa Weilder, Timo Storm, Matthias Starck
# 
# Erstelldatum: 26.10.2021
# Letzte Änderung: 11.12.2021
# 
# Titel: Informatiklabor, Projekt 4 - Gruppe 4


# imports
from time import sleep
import websocket
import json
import mysql.connector
import bot
from threading import Thread


# Dictionary zum speichern von Attributen der verschiedenen Werte, hier sparsamer als eine Klasse
grenzwerte = {
    'temperature': [24, 10, False, 'Temperatur', '°C', '🔥'],
    'humidity': [70.0, 0, False, 'Luftfeuchtigkeit', '%', '☔️'],
    'pm25': [25.0, 0, False, 'pm2.5', 'µg / m³', '🚨'],
    'pm100': [25.0, 0, False, 'pm10', 'µg / m³', '🚨']
}


# ========Microcontroller stuff=====================

# Funktion zum Abrufen der aktuelle Messdaten in form eines JSON-Strings, gibt Dictionary zurück
def getData():
    # Verbinde mit WebSocket-Server
    ws = websocket.WebSocket()
    ws.connect("ws://192.168.169.122")

    # Erhalte Daten vom Server
    data = json.loads(ws.recv())

    # Schließe die Verbindung
    ws.close()
    return data


# Erstellt die Verbindung zur Datenbank, gibt diese zurück
def connection():
    db = mysql.connector.connect(
        host="192.168.169.21",
        user="Labor2021",
        password="loveit",
        database="testdb"
    )
    return db


# Funktion zum Speichern der Daten in der Datenbank, 
# nimmt das vom Server erhaltene Dictionary als Argument, kein Rückgabewert
def insertData(data):
    with connection() as db:
        c = db.cursor()
        com = "INSERT INTO weather_uair VALUES(0, %s, %s, %s, %s, %s)"
        val = [data["timestamp"]]
        for value in data["unfiltered"].values():
            val.append(value)
        c.execute(com, val)
        db.commit()

        c = db.cursor()
        com = "INSERT INTO weather_fair VALUES(0, %s, %s, %s, %s, %s)"
        val = [data["timestamp"]]
        for value in data["filtered"].values():
            val.append(value)
        c.execute(com, val)
        db.commit()


# =====================Bot stuff===========================

# Funktion zum abrufen des aktuellesten Eintrages in der Datenbank, gibt Touple zurück
def selectLast():
    with connection() as db:
        c = db.cursor()
        c.execute("SELECT * FROM weather_fair ORDER BY timestamp DESC ")
        res = c.fetchone()
    return res


# Funktion überprüft, ob die aktuellen Werte im Gültigkeitsbereich liegen, 
# wenn nicht wird über den Telegram-Bot eine Warnung geschickt
# nimmt Dictionary der aktuellen werte als Argument, keine Rückgabewerte
def bot_sends_warning(data):
    for key in data.keys():
        if grenzwerte[key][0] > data[key] > grenzwerte[key][1]:
            msg = f"{grenzwerte[key][3]}-Messwerte wieder optimal: {data[key]} {grenzwerte[key][4]} ✅"
            if grenzwerte[key][2]:
                bot.send_message(text=msg)
                grenzwerte[key][2] = False
        else:
            msg = f"{grenzwerte[key][3]}-Messwerte außerhalb des gültigen Bereiches: {data[key]} {grenzwerte[key][4]} {grenzwerte[key][5]}"
            if not grenzwerte[key][2]:
                bot.send_message(text=msg)
                grenzwerte[key][2] = True


# Funktion zum Reagieren auf Anfragen des Botnutzers
# Nimmt Dictionary der aktuellen Werte und den Text der Nachricht des Nutzers als Argument, keine Rückgabewert
def bot_reacts(data, msg):
    msg = msg.lower()
    if msg == 'aktuelle werte':
        bot.send_message(temp=data['temperature'], hum=data['humidity'], pm25=data['pm25'], pm100=data['pm100'])
    elif msg == 'temperatur':
        bot.send_message(temp=data['temperature'])
    elif msg == 'luftfeuchtigkeit':
        bot.send_message(hum=data['humidity'])
    elif msg == 'feinstaub':
        bot.send_message(pm25=data['pm25'], pm100=data['pm100'])


current = {}


def botHandler():
    msg_index = 0 
    while True:
        global current
        if current != {}:
            filtered = current['filtered']
            bot_sends_warning(filtered)                                     # Ruft Warnfunktion des Bots auf
            try:
                msg, index = bot.get_message()                              # Ruft Inhalt und Index der letzten Nachricht des Nutzers ab
                if index > msg_index:                                       # Guckt ob Nachricht schon bearbeitet wurde
                    msg_index = index                                       # Aktualisiert letzten Index
                    bot_reacts(filtered, msg)                               # Ruft Reaktionsfunktion des Bots auf
            except:
                pass


def dataHandler():
    while True:
        global current
        data = getData()
        for key in data.keys():
            if key != 'timestamp':
                for value in grenzwerte.keys():
                    data[key][value] = round(data[key][value], 2)
        current = data


def sqlHandler():
    while True:
        global current
        if current != {}:
            insertData(current)
            sleep(30)


t1 = Thread(target=botHandler)
t2 = Thread(target=dataHandler)
t3 = Thread(target=sqlHandler)
t1.start()
t2.start()
t3.start()
print('running...')


