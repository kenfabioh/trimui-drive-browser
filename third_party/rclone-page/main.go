// Minimal rclone command frontend. MIT, same license as upstream rclone.
package main

import (
	_ "github.com/rclone/rclone/backend/drive"
	"github.com/rclone/rclone/cmd"
	_ "github.com/rclone/rclone/cmd/backend"
	_ "github.com/rclone/rclone/cmd/version"
	"github.com/rclone/rclone/fs"
)

func main() { fs.Version = "v1.69.1-drivebrowser-page.1"; cmd.Main() }
