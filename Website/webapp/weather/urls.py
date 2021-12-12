from django.urls import path
from . import views

# Legt fest, welche funktionen zu welcher URL gehören. URL bekommen 'Spitznamen' zum leichteren Aufrufen
app_name = 'weather'
urlpatterns = [
    path('', views.index_view, name='index'),
]

