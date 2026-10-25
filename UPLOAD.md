# Manual GitHub upload

## 1. Create the repository

On GitHub:
1. Open `https://github.com/new`
2. Owner: your account (`CheGanD`)
3. Repository name: `CliqueTwin`
4. Description: `Artifact repository for CliqueTwin (PVLDB/VLDB 2027)`
5. Visibility: **Public**
6. It is fine to initialize with a README, but not required.
7. Create repository.

## 2A. Recommended upload method: Git command line

Unzip this package locally, then open a terminal in the unpacked directory:

```bash
git init
git branch -M main
git add .
git commit -m "Initial public CliqueTwin artifact"
git remote add origin https://github.com/CheGanD/CliqueTwin.git
git push -u origin main
```

If the GitHub repository was initialized with a README and the push is rejected, use:

```bash
git pull --rebase origin main
git push -u origin main
```

## 2B. Web upload

For this small initial snapshot:
1. Open the new repository.
2. `Add file` -> `Upload files`.
3. Drag the **contents of the unzipped package** into GitHub.
4. Commit to `main`.

GitHub's web UI is less convenient for complex nested repositories, so Git CLI is preferred for the final artifact.

## 3. Supplemental-material URL

Use:

`https://github.com/CheGanD/CliqueTwin`

## 4. Important

This initial snapshot is not yet the complete reproducibility artifact. Before the full-paper deadline, add the final source code, run scripts, and frozen raw-result bundles (or archival links). Do not claim completion before those files are present.
