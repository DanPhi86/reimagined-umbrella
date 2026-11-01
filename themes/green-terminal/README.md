# Green Terminal – Claude-Theme

Grün auf Schwarz im Stil eines alten Phosphor-Monitors: schwarzer Hintergrund,
leuchtend grüne Monospace-Schrift, alles einfarbig.

| Rolle        | Farbe     |
|--------------|-----------|
| Hintergrund  | `#000000` |
| Text         | `#33FF33` |
| Hell / Fett  | `#8CFF8C` |
| Gedimmt      | `#1F9E1F` |
| Rahmen / Auswahl | `#1A5C1A` |

## 1. Claude Code (Terminal)

Claude Code hat keine frei definierbaren Theme-Dateien. Mit dem Theme
`dark-ansi` übernimmt es aber die 16 ANSI-Farben deines Terminals. Die
Farbschemata unten stellen alle 16 Farben auf Grüntöne, dadurch wird die
ganze Claude-Code-Oberfläche grün.

1. Farbschema für dein Terminal einrichten (Abschnitt 2).
2. In Claude Code `/theme` aufrufen und **Dark mode (ANSI colors only)** wählen
   oder `"theme": "dark-ansi"` in `~/.claude/settings.json` eintragen.
3. Optional: grüne Statuszeile
   ```bash
   cp green-statusline.sh ~/.claude/green-statusline.sh
   chmod +x ~/.claude/green-statusline.sh
   ```
   Danach den `statusLine`-Block aus `claude-code-settings.json` in
   `~/.claude/settings.json` übernehmen.

## 2. Terminal-Farbschemata

| Terminal          | Datei                       | Einrichtung |
|-------------------|-----------------------------|-------------|
| Windows Terminal  | `windows-terminal.json`     | Objekt in `"schemes"` der `settings.json` einfügen, im Profil `"colorScheme": "Green Terminal"` setzen |
| VS Code / Cursor  | `vscode-settings.json`      | Inhalt in die User-`settings.json` übernehmen |
| iTerm2            | `GreenTerminal.itermcolors` | Settings → Profiles → Colors → Color Presets → Import |
| Alacritty         | `alacritty.toml`            | In `alacritty.toml` einfügen oder per `import` einbinden |
| Kitty             | `kitty.conf`                | `include green-terminal.conf` in `kitty.conf` |

Passende Schriften: **IBM Plex Mono**, **Fira Code**; für Retro-Look **VT323**.

## 3. claude.ai im Browser

`claude-ai.user.css` ist ein UserStyle für die Browser-Erweiterung
[Stylus](https://github.com/openstyles/stylus):
Stylus installieren → *Neuen Stil erstellen* → Inhalt einfügen → speichern.
Enthält einen leichten Glüh-Effekt und Scanlines (CRT); wer die Scanlines
nicht mag, löscht den Block `body::after`.

Hinweis: claude.ai ändert seine HTML-Struktur gelegentlich. Der Stil nutzt
deshalb breite Selektoren; nach einem Update kann vereinzelt etwas nicht
grün sein.

## Hinweis zu Fehlerfarben

Das Schema ist bewusst komplett einfarbig. Rot/Grün in Diffs unterscheiden
sich daher nur in der Helligkeit. Wer Fehler deutlicher sehen will, setzt
`red` auf z. B. `#FF5F5F`.
