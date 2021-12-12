# Libraries importieren
import requests
import json


# Dictionary zum speichern von Attributen der verschiedenen Werte, hier sparsamer als eine Klasse
grenzwerte = {
    'temp': [24, 10, False, 'Temperatur', '°C', '🔥'],
    'hum': [70.0, 0, False, 'Luftfeuchtigkeit', '%', '☔️'],
    'pm25': [25.0, 0, False, 'pm2.5', 'µg / m³', '🚨'],
    'pm100': [25.0, 0, False, 'pm10', 'µg / m³', '🚨']
}


token = "2045348848:AAEFEyoIZ6r-e7I-LfR461ykUhalWPJlFT0"                            # Spezifischer Token für Bot


# Funktion zum abrufen der letzten Nachricht des Nutzers, gibt Inhalt der Nachricht und Index dieser zurück
def get_message():
    answer = requests.get(f"https://api.telegram.org/bot{token}/getUpdates")        # Get-request an Telegram-Bot-Chat, 
                                                                                    # gibt uns unten benötigte chat_id
    content = answer.content                                                        
    data = json.loads(content)                                                      # Speichern der Daten in einem Dictionary

    try:
        message = data['result'][len(data['result'])-1]['message']['text']          # Herrausfiltern der letzten Nachricht
    except:
        return ''
    return message, len(data['result'])                                             # Rückgabe des Inhalts und des Indexes


# Funktion zum Versenden von Nachrichten durch den Bot
def send_message(**args):
    # die chat_id ist die aus der obigen Response
    msg = ''
    for key in args.keys():                                                         # Fügt für jedes übergeben Argument den passenden Text 
                                                                                    # zum msg-string hinzu
        if key == 'text':
            msg = msg + args[key] + '\n'
        else:
            msg = msg + f"{grenzwerte[key][3]}: {args[key]} {grenzwerte[key][4]}\n"

    params = {"chat_id":"2057046705", "text": msg}
    url = f"https://api.telegram.org/bot{token}/sendMessage"
    message = requests.post(url, params=params)                                     # POST-request an den Telegram-Bot-Chat
    data = json.loads(message.content)

