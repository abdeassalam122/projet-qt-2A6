# Livrables modelisation - Projet Huilerie

## 1) Modele Entite / Association

- Agriculteur (1,N) LotOlive
- LotOlive (1,1) Extraction
- Machine (1,N) Extraction
- Personnel (1,N) Extraction
- Citerne (1,N) Extraction

## 2) Liste des modules (cahier de specification)

- Module Clients
- Module Citernes
- Module Reception
- Module Facturation
- Module Statistiques

## 3) Modele relationnel

- AGRICULTEUR(id_agriculteur PK, nom, prenom, tel, adresse)
- LOT_OLIVE(id_lot PK, date_reception, poids_kg, qualite, id_agriculteur FK)
- MACHINE(id_machine PK, code_machine UNIQUE, type_machine, etat)
- PERSONNEL(id_personnel PK, nom, role, tel)
- CITERNE(id_citerne PK, capacite_l, volume_actuel_l, type_huile)
- EXTRACTION(id_extraction PK, date_extraction, rendement,
  id_lot FK UNIQUE, id_machine FK, id_personnel FK, id_citerne FK)

## 4) Modele physique

Le modele physique Oracle est fourni dans :
- database/schema_oracle.sql
- database/views_oracle.sql
