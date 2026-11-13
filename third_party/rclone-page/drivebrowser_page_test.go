package drive

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/rclone/rclone/fs"
	"github.com/rclone/rclone/lib/dircache"
	"github.com/rclone/rclone/lib/pacer"
	driveapi "google.golang.org/api/drive/v3"
	"google.golang.org/api/option"
)

func TestBrowserPages(t *testing.T) {
	for _, scenario := range []string{"normal", "empty", "duplicate", "incomplete", "badtoken", "shared", "mixedduplicate"} {
		t.Run(scenario, func(t *testing.T) {
			ctx := context.Background()
			listCalls := 0
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != "GET" {
					t.Errorf("mutation attempted: %s", r.Method)
					w.WriteHeader(405)
					return
				}
				q := r.URL.Query()
				response := &driveapi.FileList{}
				if strings.Contains(q.Get("q"), "name=") {
					name := "File 000.zip"
					if strings.Contains(q.Get("q"), "File 049.zip") {
						name = "File 049.zip"
					}
					response.Files = []*driveapi.File{{Id: "unique", Name: name, MimeType: "application/zip", Md5Checksum: "abc"}}
					if q.Get("pageSize") != "2" {
						t.Errorf("boundary lookup must be bounded: %v", q)
					}
					if scenario == "duplicate" || scenario == "mixedduplicate" {
						duplicate := *response.Files[0]
						if scenario == "mixedduplicate" {
							duplicate.MimeType = driveFolderType
						}
						response.Files = append(response.Files, &duplicate)
					}
				} else {
					listCalls++
					if q.Get("pageSize") != "50" || q.Get("orderBy") != "name,folder" {
						t.Errorf("missing true page bound/order: %v", q)
					}
					if scenario == "shared" && (q.Get("driveId") != "team" || q.Get("corpora") != "drive" || q.Get("supportsAllDrives") != "true") {
						t.Errorf("shared drive options lost: %v", q)
					}
					if scenario == "badtoken" {
						http.Error(w, `{"error":{"code":400,"message":"invalid page token"}}`, 400)
						return
					}
					if scenario == "incomplete" {
						response.IncompleteSearch = true
					}
					if q.Get("pageToken") == "" {
						response.NextPageToken = "opaque+/token="
						if scenario != "empty" {
							for i := 0; i < 50; i++ {
								response.Files = append(response.Files, &driveapi.File{Id: fmt.Sprint(i), Name: fmt.Sprintf("File %03d.zip", i), MimeType: "application/zip", Size: 100, Md5Checksum: "abc"})
							}
						}
					} else {
						if q.Get("pageToken") != "opaque+/token=" {
							t.Errorf("token changed: %q", q.Get("pageToken"))
						}
						response.Files = []*driveapi.File{{Id: "50", Name: "Last.zip", MimeType: "application/zip", Size: 23, Md5Checksum: "abc"}}
					}
				}
				w.Header().Set("Content-Type", "application/json")
				_ = json.NewEncoder(w).Encode(response)
			}))
			defer server.Close()
			service, err := driveapi.NewService(ctx, option.WithHTTPClient(server.Client()), option.WithEndpoint(server.URL+"/"))
			if err != nil {
				t.Fatal(err)
			}
			f := &Fs{svc: service, ci: fs.GetConfig(ctx), rootFolderID: "root", dirCache: dircache.New("", "root", nil), dirResourceKeys: &sync.Map{}, pacer: fs.NewPacer(ctx, pacer.NewDefault(pacer.MinSleep(time.Millisecond))), opt: Options{SkipGdocs: true, ListChunk: 50}}
			if scenario == "shared" {
				f.isTeamDrive = true
				f.opt.TeamDriveID = "team"
			}
			p, err := f.browserListPage(ctx, []string{"", "50"})
			if scenario == "duplicate" || scenario == "mixedduplicate" || scenario == "incomplete" || scenario == "badtoken" {
				if err == nil {
					t.Fatal("unsafe/invalid page accepted")
				}
				return
			}
			if err != nil {
				t.Fatal(err)
			}
			expected := 50
			if scenario == "empty" {
				expected = 0
			}
			if len(p.Entries) != expected || p.NextPageToken != "opaque+/token=" || listCalls != 1 {
				t.Fatalf("fetched past first page or lost token: %d %+v", listCalls, p)
			}
			p, err = f.browserListPage(ctx, []string{p.NextPageToken, "50"})
			if err != nil {
				t.Fatal(err)
			}
			if len(p.Entries) != 1 || p.NextPageToken != "" || listCalls != 2 {
				t.Fatalf("second page wrong: %d %+v", listCalls, p)
			}
			for _, size := range []string{"0", "1001", "invalid"} {
				if _, err = f.browserListPage(ctx, []string{"", size}); err == nil {
					t.Fatal("invalid page size accepted")
				}
			}
			if listCalls != 2 {
				t.Fatal("invalid size caused network request")
			}
		})
	}
}
