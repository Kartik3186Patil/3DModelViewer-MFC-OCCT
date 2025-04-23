# 🧩 OCCT-MFC 3D Viewer

A C++ desktop application built using **MFC (Microsoft Foundation Classes)** and **Open CASCADE Technology (OCCT)** for visualizing and interacting with 3D STEP models. This project features hierarchical model parsing, original color preservation, and interactive shape highlighting—all in a native Windows SDI (Single Document Interface) application.

---

## ✨ Features

- 📂 Load and display `.STEP` files with full OCCT viewer support.
- 🧱 Visual hierarchy of assemblies and parts.
- 🎨 Preserves original colors of imported STEP shapes.
- 🖱️ Click to highlight shapes in the 3D view.
- 🔄 Restore color of previously selected shapes.
- 📜 Hierarchy traversal using `TDF_Label` and `XCAFDoc_ShapeTool`.
- 🧠 Clear backend/frontend separation with a shared data structure.
- 🪟 Built entirely in native C++ with MFC, no external GUI dependencies.

---

## 🧰 Tech Stack

- **C++** with **MFC (Microsoft Foundation Classes)**
- **Open CASCADE Technology (OCCT)** for 3D geometry handling
- **Win32** and native Windows application development
- Visual Studio Solution with two separate projects:
  - `FrontendMFC`: Handles UI, 3D view, user interaction.
  - `Backend`: Loads STEP files, builds model structure, processes shapes.

