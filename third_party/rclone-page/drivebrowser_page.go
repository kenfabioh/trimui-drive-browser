// Drive Browser extension to rclone v1.69.1. MIT, same license as rclone.
package drive

import (
	"context"
	"fmt"
	"strconv"

	driveapi "google.golang.org/api/drive/v3"
)

type browserPageRequest struct {
	Token string
	Size  int64
	Next  string
}
type browserEntry struct {
	Name     string
	IsDir    bool
	Size     int64
	MimeType string
}
type browserPage struct {
	Entries       []browserEntry
	NextPageToken string
}

// browserListPage reuses rclone's existing authenticated, read-only list path,
// encoding, shared-drive options, resource keys, shortcut handling and retries.
func (f *Fs) browserListPage(ctx context.Context, args []string) (*browserPage, error) {
	if len(args) != 2 {
		return nil, fmt.Errorf("need page token and page size")
	}
	size, err := strconv.ParseInt(args[1], 10, 64)
	if err != nil || size < 1 || size > 1000 {
		return nil, fmt.Errorf("page size must be 1..1000")
	}
	directoryID, err := f.dirCache.FindDir(ctx, "", false)
	if err != nil {
		return nil, err
	}
	req := &browserPageRequest{Token: args[0], Size: size}
	f.browserPage = req
	defer func() { f.browserPage = nil }()
	out := &browserPage{Entries: make([]browserEntry, 0)}
	_, err = f.list(ctx, []string{actualID(directoryID)}, "", false, false, f.opt.TrashedOnly, false, func(item *driveapi.File) bool {
		directory := item.MimeType == driveFolderType
		if f.opt.AuthOwnerOnly && !directory && !isAuthOwned(item) {
			return false
		}
		if !directory && item.Md5Checksum == "" {
			return false
		} // native Google Docs omitted
		out.Entries = append(out.Entries, browserEntry{item.Name, directory, item.Size, item.MimeType})
		return false
	})
	if err != nil {
		return nil, err
	}
	out.NextPageToken = req.Next
	f.browserPage = nil
	oldChunk := f.opt.ListChunk
	f.opt.ListChunk = 2
	defer func() { f.opt.ListChunk = oldChunk }()
	// Names sort first, so equal names stay adjacent across page boundaries,
	// including a folder and file (or folder shortcut) sharing the same name.
	if len(out.Entries) > 0 {
		boundaries := []string{out.Entries[0].Name}
		if last := out.Entries[len(out.Entries)-1].Name; last != boundaries[0] {
			boundaries = append(boundaries, last)
		}
		for _, name := range boundaries {
			count := 0
			_, err = f.list(ctx, []string{actualID(directoryID)}, name, false, false, f.opt.TrashedOnly, false, func(item *driveapi.File) bool { count++; return count > 1 })
			if err != nil {
				return nil, err
			}
			if count > 1 {
				return nil, fmt.Errorf("duplicate names at page boundary; rename items before downloading")
			}
		}
	}
	return out, nil
}
