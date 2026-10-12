# GitHub fork workflow

This repository is a derivative of Drommedhar's RE:INTEGRAL and preserves its
history. GitHub's fork relationship is separate from local Git ancestry.

Create the GitHub fork from https://github.com/Drommedhar/mgs-reintegral/fork,
under ImperishableNEET, preferably named `mgs1-reintegrated`. If that name is
already occupied, inspect the existing repository before renaming anything;
do not delete it or force-push over unrelated work.

Once the fork exists, connect this checkout:

```powershell
git remote add origin https://github.com/ImperishableNEET/mgs1-reintegrated.git
python tools/check_public_tree.py
git push -u origin reintegrated
```

If origin already exists, inspect its URL before changing it. Keep upstream as
`https://github.com/Drommedhar/mgs-reintegral.git`. Select `reintegrated` as the
fork's default branch. Do not force-push. No executable release is implied.

For contributions, branch from the current upstream branch and cherry-pick only
the focused change. Keep branding and unrelated experimental work out of PRs.
Preserve licenses and credit; exclude local assets, private saves and binaries.

At preparation time the local ancestry is retained, but GitHub fork creation and
origin attachment are pending authenticated access.
