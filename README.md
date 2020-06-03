# Garden Watering System

## EN Version :

**Principle of operation :**

 - The system is composed of a master module, this can be an ESP8266 (ESP-12E) or an ESP32, on which an RTC module is connected (DS3231 in my case).
 - Solenoid valves can be connected directly to the master module (via an H-bridge for latching valves, an L293D is perfect because only a short impulse is needed to open/close the valve) or remotely thanks to children modules (which are also ESP8266 but less demanding, it can be a very small ESP-01 alone or with an arduino to have more of GPIOs).
 - A web interface is located in the SPIFFS memory of the master module and allows to create/delete cycles/valves.
 - The cycles are adjustable with a start time and an end time, and it is possible to select the days of activation. It's also possible to create several cycles for a single solenoid valve.

**Note:**

The program is far from being finished, I started it not very long ago and I plan to add several features in the future (ex: rain gauge)
It is not commented/documented at all at the moment... It must be functional for this summer (essential for my garden :) )
Everything has been coded on PlatformIO for Visual Studio Code, a very good alternative to the Arduino IDE I find.

## FR Version :

**Principe de fonctionnement :**
 - Le système est composé d'un module maître, cela peut être un ESP8266 (ESP-12E) ou un ESP32, sur lequel est connecté un module RTC (DS3231 dans mon cas)
 -  Il est possible de connecter des électrovannes directement sur le module maître (via un pont en H pour les vannes à vérouillage, un L293D est parfait car il suffit d'envoyer une courte impulsion pour ouvrir/fermer la vanne) ou bien à distance grâce à des modules enfants (qui sont aussi des ESP8266 mais moins exigeant, cela peut être un tout petit ESP-01 seul ou avec un arduino pour avoir plus de sorties)
 -  Une interface Web se trouve dans la mémoire SPIFFS du module maître et permet de créer/supprimer des cycles/électrovannes
 -  Les cycles sont réglables avec une heure de début et une heure de fin, et il est possible de sélectionner les jours d'activation. Il est aussi possible de créer plusieurs cycles pour une seule électrovanne.

**A noter :**

Le programme est loin d'être terminé, je l'ai commencé il y a pas très longtemps et je compte ajouter plusieurs fonctionnalités à l'avenir (ex : pluviomètre)
Il n'est pas du tout commenté/documenté pour l'instant... Il faut qu'il soit fonctionnel pour cet été (indispensable pour mon jardin :) )
Tout a été codé sur PlatformIO pour Visual Studio Code, une très bonne alternative à l'IDE Arduino je trouve.
