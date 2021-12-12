from django.db import models


# Django Models als Python Klassen zum Laden von Datenbankeinträgen
class uAir(models.Model):
    timestamp = models.DateTimeField()          # Datentyp DateTime
    temperature = models.FloatField()           # Datentyp Float
    humidity = models.FloatField()              # Datentyp Float
    pm25 = models.FloatField()                  # Datentyp Float
    mp100 = models.FloatField()                 # Datentyp Float


class fAir(models.Model):
    timestamp = models.DateTimeField()          # Datentyp DateTime
    temperature = models.FloatField()           # Datentyp Float
    humidity = models.FloatField()              # Datentyp Float
    pm25 = models.FloatField()                  # Datentyp Float
    mp100 = models.FloatField()                 # Datentyp Float
