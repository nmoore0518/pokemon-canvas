# Releasing Pokémon Canvas

Two repositories:

- **gym-trainer-hack** (private): where the work happens. Every push to `main` builds the ROM for
  testing (Actions → the latest **Build ROM** run → **gym-trainer-hack-rom**). Only Nick sees it.
- **pokemon-canvas** (public): the code, as of each release, and the releases themselves: a BPS
  patch per version. This is the one to link from Romhacking.net, Reddit and so on. Its
  [latest release](https://github.com/nmoore0518/pokemon-canvas/releases/latest) link always points
  at the newest patch.

## Releasing a version

Push a tag named for the version (`v0.1`, `v0.2`, `v0.2.1`, ...) to **gym-trainer-hack**. Easiest on
github.com: **Releases → Draft a new release → Choose a tag**, type the new version (e.g. `v0.2`),
**Create new tag on publish**, target `main`, then **Publish release**. (Or ask Claude to tag it.)

That starts the **Release** workflow (Actions tab, about 15 minutes). It builds the release version
(no debug menus), makes `pokemon-canvas-v0.2.bps` against the clean FireRed, copies the code to
**pokemon-canvas**, and publishes the release there with the changes since the last version
(the commit messages) and patching instructions.

If it fails partway, fix the problem and run it again: Actions → **Release** → **Run workflow**,
with the same tag.

## One-time setup

Do these in order.

1. **Create the public repo.** github.com → **+** → **New repository**. Name: `pokemon-canvas`.
   Public. Leave "Add a README", .gitignore and license **unchecked** (it must start empty).
2. **Make a token that can write to it.** Your profile picture → **Settings** → **Developer
   settings** → **Personal access tokens** → **Fine-grained tokens** → **Generate new token**.
   - Name: `pokemon-canvas releases`. Expiration: up to a year (put a reminder to renew it).
   - Repository access: **Only select repositories** → `pokemon-canvas`.
   - Permissions → Repository permissions: **Contents: Read and write**, **Workflows: Read and
     write**. (Metadata: Read-only is added on its own.)
   - **Generate token**, and copy it (GitHub shows it only once).
3. **Give it to the private repo.** In **gym-trainer-hack**: **Settings** → **Secrets and
   variables** → **Actions** → **New repository secret**. Name: `PUBLIC_REPO_TOKEN`. Paste the
   token. **Add secret**.
4. **Make gym-trainer-hack private.** **Settings** → **General** → scroll to **Danger Zone** →
   **Change repository visibility** → **Private**. (Do this before step 5.)
5. **Upload the clean FireRed.** In **gym-trainer-hack**: **Releases** → **Draft a new release** →
   **Choose a tag** → type `base-rom` → **Create new tag on publish**. Title: `Base ROM (private)`.
   Drag in your **Pokémon FireRed (USA) Rev 1 (v1.1)** `.gba`. Tick **Set as a pre-release**.
   **Publish release**. The Release workflow checks it's the right ROM.
6. **Optional, in pokemon-canvas:** **Settings** → **Actions** → **General** → **Disable actions**
   (the workflows skip themselves there anyway), and make sure **Issues** is ticked under
   **Settings** → **General** → **Features** if you want bug reports there too.

## Things to know

- Private repos get 2,000 free GitHub Actions minutes a month (public ones are unlimited). A build
  takes about 10-15 minutes, so that's well over 100 builds a month.
- The patch is made against FireRed (USA) Rev 1 (v1.1), SHA1
  `dd5945db9b930750cb39d00c84da8571feebf417`, so that's the only ROM players can patch.
- Players keep their saves across versions: patch the clean ROM again, same file name as before.
