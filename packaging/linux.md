# Signalbash x Linux


## Installation

To install the Signalbash plugin, 

- Download the appropriate zip file for your CPU architecture (either x86_64/x64 or ARM64/AARCH64)
- Unzip the zip file
- Copy or move the CLAP/VST3 plugins to their proper directories

### CLAP

For CLAP plugins, this is typically one of the following directories:

- `~/.clap`
- `/usr/lib/clap`


### VST3

For VST3 plugins, common locations include:

- `~/.vst3`
- `/usr/lib/vst3/`
- `/usr/local/lib/vst3/`


## After Installation

Once you've placed the plugins in their destination directories, they should
automatically be recognized by your DAW. However, if your DAW was open during
installation, you will have to rescan your plugins for Signalbash to appear.


## Uninstalling

To uninstall, delete the plugin files from the locations where you
installed the plugins.


## Usage & Documentation

For more information about usage, please visit the official Signalbash documetation,
available at the following link:

https://info.signalbash.com/


## License

The Signalbash Plugin is licensed under the GNU Affero General Public License, Version 3.
A copy of the license should be included with this program.
If not, see: https://www.gnu.org/licenses/

To obtain a copy of the source, visit: https://github.com/signalbash-audio/signalbash


## Command Line Installation Example

```
# Assumes you've downloaded the zip file to the current directory
# and `unzip` is installed on your machine

# CLAP Destination, change if installing for all users
CLAP_DEST=~/.clap  # or `/usr/lib/clap` (might require sudo permissions)

# VST3 Destination, change if installing for all users
VST3_DEST=~/.vst3  # or "/usr/local/lib/vst3/" (might will require sudo permissions)

# Create directories if they don't exist
mkdir -p "${CLAP_DEST}" "${VST3_DEST}"

unzip 'Signalbash-*.zip' -d signalbash_plugin
cd signalbash_plugin

cp Signalbash.clap "${CLAP_DEST}"
echo "Copied Signalbash CLAP plugin to CLAP Plugin Directory"

cp -r Signalbash.vst3 "${VST3_DEST}"
echo "Copied Signalbash VST3 plugin to VST3 Plugin Directory"

echo "Installation Complete"
```
