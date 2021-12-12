from django.shortcuts import render                             # Benötigte Imports
from plotly.offline import plot
from plotly.graph_objs import Scatter, Bar
from .models import uAir, fAir


def index_view(request):
    current = fAir.objects.order_by('-timestamp')[0]            # Lädt zuletzt gespeichertes Element aus der Datenbank, entsprechender SQL-Befehl: SELECT * FROM weather_fair SORT BY timestamp DESC
    entries = [fAir.objects.order_by('-timestamp')[0:30],       # Lädt die 30 zuletzt gespeicherten Elemente aus der Datenbank
               uAir.objects.order_by('-timestamp')[0:30]]

    t_graphs = []                                               # Arrays zum Speichern der Plotly-Daten
    h_graphs = []
    pm_graphs = []

    for i in range(2):
        x_data = []                                             # Arrays zum Sortieren der Daten nach Messwert
        temp = []
        humidity = []
        p25 = []
        p100 = []
        for x in entries[i]:
            x_data.append(x.timestamp)                          # Zuweisung der Messwerte der einzelnen Einträge zum passenden Array
            temp.append(x.temperature)
            humidity.append(x.humidity)
            p25.append(x.pm25)
            p100.append(x.mp100)

        print(humidity)

        if i == 0:                                              # Unterscheidung welcher Name dem Datensatz zugeordnet werden muss
            name = 'gefiltert'
        else:
            name = 'ungefiltert'

        t_graphs.append(Scatter(x=x_data, y=temp, mode='lines+markers', name=name, opacity=1, marker_size=10))                                  # Ploten der Datensätze mit Plotly
        h_graphs.append(Bar(x=x_data, y=humidity, name=name, opacity=1))
        pm_graphs.append(Scatter(x=x_data, y=p25, mode='lines+markers', name='pm2.5 ' + name, opacity=1, marker_size=10))
        pm_graphs.append(Scatter(x=x_data, y=p100, mode='lines+markers', name='pm10 ' + name, opacity=1, marker_size=10))

    plot_temp = plot({'data': t_graphs, 'layout': {'xaxis_title': 'Time', 'yaxis_title': 'Temperatur / °C'}}, output_type='div')               # Transformieren der Plots zur einfacheren Einbindung in das HTML Dokument
    plot_hum = plot({'data': h_graphs, 'layout': {'xaxis_title': 'Time', 'yaxis_title': 'Luftfeuchtigkeit / %'}}, output_type='div')
    plot_p25 = plot({'data': pm_graphs, 'layout': {'xaxis_title': 'Time', 'yaxis_title': 'Feinstaub / µg/m³'}}, output_type='div')

    return render(request, 'weather/index.html', {'current': current, 'plot_temp': plot_temp, 'plot_hum': plot_hum, 'plot_p25': plot_p25})     # Aufrufen des zu rendernden HTML-Dokuments und Übergabe der Graphen an dieses
