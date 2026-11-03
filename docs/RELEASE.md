# Release procedure

The workflow `.github/workflows/release.yml` makes a GitHub release from each version tag (T14-07, REQ-14-5).

## Make a release

1. Make sure that CI passes on `main`.
2. Change the version in the root `CMakeLists.txt` (`project(AlgoRythm VERSION x.y.z)`).
3. Make a tag in the form `v*.*.*` and push it:

   ```bash
   git tag v0.1.0-rc1
   git push origin v0.1.0-rc1
   ```

4. The workflow makes the release, then attaches:
   - `Algo-Rythm-<tag>-macOS.dmg`: a universal binary (arm64 and x86_64) with `algo-cli`.
   - `Algo-Rythm-<tag>-Windows.zip`: the app and `algo-cli.exe`.
5. A tag with a hyphen, for example `v0.1.0-rc1`, makes a pre-release.

The release notes come from `docs/RELEASE_NOTES.md`. Change that file before you push the tag.

## Unsigned builds

If you have no Apple Developer ID, the workflow makes unsigned builds. The release notes tell the user how to open an unsigned app.
Write this limit in the release notes of each unsigned release.

## Code signing and notarization on macOS

You need an Apple Developer ID Application certificate and an app-specific password for your Apple ID.

1. Export the certificate and its private key from Keychain Access as a `.p12` file.
2. Add these repository secrets in GitHub (Settings > Secrets and variables > Actions):
   - `MACOS_CERTIFICATE`: the `.p12` file as base64 (`base64 -i cert.p12`).
   - `MACOS_CERTIFICATE_PASSWORD`: the password of the `.p12` file.
   - `MACOS_SIGNING_IDENTITY`: for example `Developer ID Application: Your Name (TEAMID)`.
   - `APPLE_ID`, `APPLE_TEAM_ID` and `APPLE_APP_PASSWORD`: for `notarytool`.
3. In the `macos` job, before the package step, add steps that:
   1. Make a temporary keychain and import the certificate:

      ```bash
      security create-keychain -p "$RUNNER_TEMP" build.keychain
      security import cert.p12 -k build.keychain -P "$MACOS_CERTIFICATE_PASSWORD" -T /usr/bin/codesign
      security set-key-partition-list -S apple-tool:,apple: -s -k "$RUNNER_TEMP" build.keychain
      security list-keychains -s build.keychain
      ```

   2. Sign the app and the CLI with the hardened runtime:

      ```bash
      codesign --force --deep --options runtime --timestamp --sign "$MACOS_SIGNING_IDENTITY" Algo-Rythm.app
      codesign --force --options runtime --timestamp --sign "$MACOS_SIGNING_IDENTITY" algo-cli
      ```

4. After the package step, notarize and staple the `.dmg` file:

   ```bash
   xcrun notarytool submit Algo-Rythm.dmg --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
   xcrun stapler staple Algo-Rythm.dmg
   ```

5. Check the result with `spctl --assess --type open --context context:primary-signature -v Algo-Rythm.dmg`.

## Code signing on Windows

1. Get a code signing certificate (OV or EV) as a `.pfx` file.
2. Add the secrets `WINDOWS_CERTIFICATE` (base64) and `WINDOWS_CERTIFICATE_PASSWORD`.
3. Before the package step, sign each `.exe` file:

   ```powershell
   signtool sign /f cert.pfx /p $env:WINDOWS_CERTIFICATE_PASSWORD /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 Algo-Rythm.exe algo-cli.exe
   ```

## The plugin

The release workflow does not ship the plugin yet. CI builds the VST3 on macOS and Windows and runs `pluginval` at strictness level 5 (T16-03).
To build the plugin, use `-DALGO_BUILD_PLUGIN=ON`. Before you ship it, sign it with the same steps as the app.
