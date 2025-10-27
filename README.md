Geo Trinity is a multiplayer 2D minimalist geometric bullet-hell game with the super sacred saint trinity of Tank Heal Dps.
You'll play a shade according to your class (Square for tank, circle for heal, triangle for dps) 
According to this form you'll perform actions by splitting yourself in order to help each other and defeat the bosses.

This project is made on Unreal 5, it does not use Chaos neither much actor replication. 
All inputs are send to the server and dispatch to the clients. So lag is not an issue theorically :')
All collisions are calculated locally and on the server by simple math, so it cost nothing to run.
Visually... I don't know for now


Tech : 
- Replication Unreal des ennemies et des projectils/actions.
- Replication personnelle des joueurs.
-   Stocker l'etat des joueurs (Position orientation velocite)
-   Rollback, rejouer les frames.
- GAS pour tous les spells.
