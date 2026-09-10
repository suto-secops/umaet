# Umaet 💰

**Umaet** is a native KDE Plasma personal finance and budget tracking application built with C++20, Qt 6, and KDE Kirigami. It is designed to be lightweight, fast, and completely offline-first with an intuitive user interface that conforms to KDE human interface guidelines (HIG).

![Umaet Icon](icons/umaet.svg)

---

## ✨ Features

- **Transaction Management**: Record daily expenses and income with customizable categories, dates, and notes.
- **Visual Spending Trends**: Aggregated statistics and bar graphs comparing money earned vs. spent by week, month, and year.
- **Monthly Budgeting**:
  - Global monthly spending cap with instant progress indicators (On Track, Warning, Over-Budget).
  - Individual category allocations (e.g. Groceries, Rent, Utilities) with remaining balance alerts.
- **Bulk Import & Export**: Import and export transactions seamlessly using standard CSV or JSON formats.
- **Native KDE Integration**: Follows your system's Breeze color palette (auto dark/light mode), desktop shortcuts, and AppStream metadata.
- **ACID-Compliant Local Storage**: All data is securely stored on your device in SQLite (`~/.local/share/umaet/umaet.db`).

---

## 🛠️ Build & Installation

### Requirements (Arch Linux / KDE Plasma)

```bash
sudo pacman -S cmake extra-cmake-modules ninja gcc qt6-base qt6-declarative kirigami kcoreaddons ki18n sqlite
```

### Building from Source

```bash
git clone https://github.com/suto-secops/umaet.git
cd umaet
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

### Running Locally without Installing

```bash
./build/umaet
```

---

## 📄 License

This project is licensed under the **PolyForm Noncommercial License 1.0.0** ([LICENSE](LICENSE)).

