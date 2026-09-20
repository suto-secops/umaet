import sqlite3
from pathlib import Path

DB = Path.home() / ".local/share/umaet/umaet.db"
con = sqlite3.connect(DB)
cur = con.cursor()

# Simple heuristic: take the first 3 words of the note as the title
cur.execute("SELECT id, note FROM transactions WHERE title = ''")
rows = cur.fetchall()

for row_id, note in rows:
    title = " ".join(note.split()[:3]) if note else "Unknown"
    cur.execute("UPDATE transactions SET title = ? WHERE id = ?", (title, row_id))

con.commit()
con.close()
print(f"Updated titles for {len(rows)} transactions.")
