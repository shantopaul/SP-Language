# SP Lang — Official Logo Assets & Downloads

This directory contains the official logo suite for **SP Lang**, synchronized with the Figma project:
[SP-Lang-All-Logo on Figma](https://www.figma.com/design/i84QHJ5bj642RvqtfzOws5/SP-Lang-All-Logo?node-id=0-1&p=f&t=5TqdD5d2dtFGfh6d-0)

---

## 🚀 How to Download All High-Quality Logos

### Method 1: 1-Click Export from Figma (Fastest)
1. In your Figma browser tab, select all the frames on the canvas (or press `Ctrl + A`).
2. In the right-hand sidebar at the bottom, find the **Export** section.
3. Click `+` and choose **SVG** (vector, infinite scaling) and **4x PNG** (Ultra HD raster).
4. Click **Export layers**.
5. Run the automatic importer script from PowerShell:
   ```powershell
   .\import_from_downloads.ps1
   ```
   *This automatically detects the exported files or zip in your `Downloads` folder and organizes them here.*

---

### Method 2: Automated Download via Figma API
If you have a Figma Personal Access Token:
1. In Figma, go to **Account Settings** -> **Personal Access Tokens** -> **Generate New Token**.
2. Run:
   ```powershell
   .\download_figma_logos.ps1 -Token "YOUR_FIGMA_TOKEN"
   ```
   *The script will query the Figma REST API for all 12 frames in `SP-Lang-All-Logo` and download every SVG and 4x PNG automatically.*

---

## 🔌 Figma MCP Configuration

The Figma Model Context Protocol (MCP) server has been added to your global configuration at:
`C:\Users\Shanto Paul\.gemini\config\mcp_config.json`:

```json
{
  "mcpServers": {
    "Figma": {
      "serverUrl": "https://mcp.figma.com/mcp",
      "url": "https://mcp.figma.com/mcp"
    }
  }
}
```

---

## 📁 Logo Variant Inventory

1. `Primary logo` (Standard horizontal brandmark with full color icon + wordmark)
2. `Icon brandmark` (Standalone isometric geometric glyph)
3. `Horizontal logo lockup` (Balanced navbar lockup)
4. `Wordmark only` ("SP Lang" stylized lettering)
5. `App icon brandmark` (Squircle app container)
6. `Dark background version` (High-contrast for dark IDEs, CLI & website)
7. `Monochrome white version` (Pure `#FFFFFF` silhouette for dark prints)
8. `Monochrome black version` (Pure `#000000` silhouette for black & white documents)
9. `App icon on white` (Light mode desktop / mobile app icon)
10. `App icon on navy` (Dark mode desktop / mobile app icon)
11. `Custom lettering specimen` (Typography specimen sheet)
12. `Brand Color` (Official palette swatches)
