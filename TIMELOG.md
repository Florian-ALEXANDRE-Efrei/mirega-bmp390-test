# TIMELOG

> Les durées sont approximatives et incluent la recherche d’informations (datasheets, exemples, docs en ligne)
> ainsi que la prise en main de Copilot / VS Code.

| Date       | Durée estimée | Tâche                                                                 |
|------------|---------------|-----------------------------------------------------------------------|
| 2025-12-06 | 1h30          | Lecture détaillée de l’énoncé, prise de notes, compréhension des 2 parties |
| 2025-12-06 | 2h00          | Recherche web sur BMP390, BMP3_SensorAPI, exemples d’utilisation I2C/Linux |
| 2025-12-06 | 2h30          | Analyse du driver Bosch (bmp3.c/h/defs.h), identification des fonctions clés et workflow minimal |
| 2025-12-07 | 3h00          | Design et implémentation du wrapper C++ `bmp390::Bmp390` (header + cpp) |
| 2025-12-07 | 1h00          | Écriture de l’exemple `bmp390_lib/examples/bmp390_example.cpp` et ajustements |
| 2025-12-07 | 1h00          | Rédaction du README de la librairie BMP390 (structure, exemple, limites) |
| 2025-12-08 | 1h30          | Conception de l’architecture multi-capteurs (interface ISensor, classes concrètes) |
| 2025-12-08 | 1h00          | Rédaction de `docs/ARCHITECTURE_MULTISENSOR.md` (justification + alternatives) |
| 2025-12-08 | 1h30          | Écriture de `examples/multisensor_example.cpp` (pseudo-code multi-capteurs, alarme) |
| 2025-12-08 | 1h00          | Relecture globale du code (noms, commentaires, TODO) et petits refactors |
| 2025-12-08 | 1h00          | Rédaction du README racine + mise à jour de `docs/NOTES.md` et du TIMELOG |
| 2025-12-08 | 1h30          | Recherche web complémentaire (patterns d’architecture capteurs, RTOS, alarmes, bonnes pratiques C++/embedded) |
| 2025-12-08 | 1h30          | Prise en main de GitHub Copilot (agents, chat, intégration VS Code) et itérations avec l’IA sur le design |