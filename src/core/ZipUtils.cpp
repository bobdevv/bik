#include "core/ZipUtils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

namespace bik {

std::vector<std::string> ZipUtils::listFiles(const std::string& dir) {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing files: " << e.what() << std::endl;
    }
    
    return files;
}

size_t ZipUtils::getFileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (const std::exception& e) {
        return 0;
    }
}

bool ZipUtils::deleteDirectory(const std::string& path) {
    try {
        return fs::remove_all(path) > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting directory: " << e.what() << std::endl;
        return false;
    }
}

bool ZipUtils::copyDirectory(const std::string& source, const std::string& dest) {
    try {
        fs::copy(source, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error copying directory: " << e.what() << std::endl;
        return false;
    }
}

#if defined(BIK_HAVE_LIBZIP)

#include <zip.h>

static bool ensure_parent_dir(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    return !ec;
}

static std::string to_unix_path(const fs::path& p) {
    std::string s = p.generic_string();
    // Ensure no leading ./
    if (s.rfind("./", 0) == 0) s = s.substr(2);
    return s;
}

bool ZipUtils::createZip(const std::string& sourceDir, const std::string& zipPath) {
    fs::path source = fs::absolute(sourceDir);
    fs::path dest = fs::absolute(zipPath);
    if (!fs::exists(source)) {
        std::cerr << "Source directory does not exist: " << source << std::endl;
        return false;
    }

    fs::create_directories(dest.parent_path());

    int errorp = 0;
    zip_t* za = zip_open(dest.string().c_str(), ZIP_TRUNCATE | ZIP_CREATE, &errorp);
    if (!za) {
        std::cerr << "libzip: failed to open archive for writing (error " << errorp << ")\n";
        return false;
    }

    bool ok = true;
    try {
        for (auto it = fs::recursive_directory_iterator(source); it != fs::recursive_directory_iterator(); ++it) {
            const auto& entry = *it;
            fs::path path = entry.path();

            // Exclude .bik directory
            fs::path rel = fs::relative(path, source);
            if (!rel.empty() && rel.begin()->string() == ".bik") {
                if (entry.is_directory()) it.disable_recursion_pending();
                continue;
            }

            if (entry.is_directory()) {
                // Optionally add explicit directory entry
                continue;
            }

            if (entry.is_regular_file()) {
                std::string rel_unix = to_unix_path(rel);
                zip_source_t* zs = zip_source_file(za, path.string().c_str(), 0, 0);
                if (!zs) {
                    std::cerr << "libzip: zip_source_file failed for " << path << "\n";
                    ok = false;
                    break;
                }
                zip_int64_t idx = zip_file_add(za, rel_unix.c_str(), zs, ZIP_FL_OVERWRITE);
                if (idx < 0) {
                    std::cerr << "libzip: zip_file_add failed for " << rel_unix << ": " << zip_strerror(za) << "\n";
                    zip_source_free(zs);
                    ok = false;
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error while zipping: " << e.what() << std::endl;
        ok = false;
    }

    if (zip_close(za) != 0) {
        std::cerr << "libzip: zip_close failed: archive not written\n";
        ok = false;
    }

    if (!ok) {
        // Remove incomplete archive
        std::error_code ec;
        fs::remove(dest, ec);
    }
    return ok;
}

bool ZipUtils::extractZip(const std::string& zipPath, const std::string& destDir) {
    fs::path zip_file = fs::absolute(zipPath);
    fs::path dest = fs::absolute(destDir);
    if (!fs::exists(zip_file)) {
        std::cerr << "Zip file does not exist: " << zip_file << std::endl;
        return false;
    }
    fs::create_directories(dest);

    int errorp = 0;
    zip_t* za = zip_open(zip_file.string().c_str(), ZIP_RDONLY, &errorp);
    if (!za) {
        std::cerr << "libzip: failed to open archive (error " << errorp << ")\n";
        return false;
    }

    bool ok = true;
    zip_int64_t n = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < n; ++i) {
        struct zip_stat st;
        zip_stat_init(&st);
        if (zip_stat_index(za, i, 0, &st) != 0) {
            ok = false; break;
        }
        std::string name = st.name ? st.name : "";
        if (name.empty()) continue;

        fs::path out_path = dest / fs::path(name);
        if (name.back() == '/') {
            // Directory entry
            std::error_code ec; fs::create_directories(out_path, ec);
            continue;
        }

        if (!ensure_parent_dir(out_path)) { ok = false; break; }

        zip_file_t* zf = zip_fopen_index(za, i, 0);
        if (!zf) { ok = false; break; }

        std::ofstream ofs(out_path, std::ios::binary);
        if (!ofs) { zip_fclose(zf); ok = false; break; }

        char buf[1 << 15];
        zip_int64_t read = 0;
        while ((read = zip_fread(zf, buf, sizeof(buf))) > 0) {
            ofs.write(buf, static_cast<std::streamsize>(read));
        }
        zip_fclose(zf);
        ofs.close();

        if (read < 0) { ok = false; break; }
    }

    zip_close(za);
    return ok;
}

#elif defined(BIK_HAVE_MINIZIP)

// Implementation using classic Minizip (zip/unzip). Depending on your platform,
// headers may be <zip.h>/<unzip.h> or <minizip/zip.h>/<minizip/unzip.h>.

extern "C" {
#include <zip.h>
#include <unzip.h>
}

static bool write_stream_to_file(unzFile uf, const fs::path& out_path) {
    std::error_code ec; fs::create_directories(out_path.parent_path(), ec);
    std::ofstream ofs(out_path, std::ios::binary);
    if (!ofs) return false;
    const size_t BUFSIZE = 1 << 15;
    std::vector<char> buf(BUFSIZE);
    int read = 0;
    while ((read = unzReadCurrentFile(uf, buf.data(), static_cast<unsigned int>(buf.size()))) > 0) {
        ofs.write(buf.data(), read);
    }
    ofs.close();
    return read >= 0;
}

static bool add_file_to_zip(zipFile zf, const fs::path& abs_path, const fs::path& rel_path) {
    std::string rel_unix = rel_path.generic_string();
    zip_fileinfo zi{};
    if (zipOpenNewFileInZip(zf, rel_unix.c_str(), &zi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
        return false;
    }
    std::ifstream ifs(abs_path, std::ios::binary);
    if (!ifs) { zipCloseFileInZip(zf); return false; }
    const size_t BUFSIZE = 1 << 15;
    std::vector<char> buf(BUFSIZE);
    while (ifs) {
        ifs.read(buf.data(), buf.size());
        std::streamsize g = ifs.gcount();
        if (g > 0) {
            if (zipWriteInFileInZip(zf, buf.data(), static_cast<unsigned int>(g)) != ZIP_OK) {
                zipCloseFileInZip(zf);
                return false;
            }
        }
    }
    return zipCloseFileInZip(zf) == ZIP_OK;
}

bool ZipUtils::createZip(const std::string& sourceDir, const std::string& zipPath) {
    fs::path source = fs::absolute(sourceDir);
    fs::path dest = fs::absolute(zipPath);
    if (!fs::exists(source)) {
        std::cerr << "Source directory does not exist: " << source << std::endl;
        return false;
    }
    fs::create_directories(dest.parent_path());

    zipFile zf = zipOpen(dest.string().c_str(), APPEND_STATUS_CREATE);
    if (!zf) { std::cerr << "minizip: cannot open archive for writing\n"; return false; }

    bool ok = true;
    try {
        for (auto it = fs::recursive_directory_iterator(source); it != fs::recursive_directory_iterator(); ++it) {
            const auto& entry = *it;
            fs::path path = entry.path();
            fs::path rel = fs::relative(path, source);
            if (!rel.empty() && rel.begin()->string() == ".bik") {
                if (entry.is_directory()) it.disable_recursion_pending();
                continue;
            }
            if (entry.is_directory()) continue;
            if (entry.is_regular_file()) {
                if (!add_file_to_zip(zf, path, rel)) { ok = false; break; }
            }
        }
    } catch (...) { ok = false; }

    if (zipClose(zf, nullptr) != ZIP_OK) ok = false;
    if (!ok) { std::error_code ec; fs::remove(dest, ec); }
    return ok;
}

bool ZipUtils::extractZip(const std::string& zipPath, const std::string& destDir) {
    fs::path zip_file = fs::absolute(zipPath);
    fs::path dest = fs::absolute(destDir);
    if (!fs::exists(zip_file)) {
        std::cerr << "Zip file does not exist: " << zip_file << std::endl;
        return false;
    }
    fs::create_directories(dest);

    unzFile uf = unzOpen(zip_file.string().c_str());
    if (!uf) { std::cerr << "minizip: cannot open archive\n"; return false; }

    bool ok = true;
    if (unzGoToFirstFile(uf) != UNZ_OK) { unzClose(uf); return false; }
    do {
        if (unzOpenCurrentFile(uf) != UNZ_OK) { ok = false; break; }
        unz_file_info fi{}; char filename[1024];
        if (unzGetCurrentFileInfo(uf, &fi, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK) { ok = false; break; }
        std::string name(filename);
        fs::path out_path = dest / fs::path(name);
        if (name.back() == '/') {
            std::error_code ec; fs::create_directories(out_path, ec);
            unzCloseCurrentFile(uf);
        } else {
            if (!write_stream_to_file(uf, out_path)) { ok = false; break; }
            unzCloseCurrentFile(uf);
        }
    } while (ok && unzGoToNextFile(uf) == UNZ_OK);

    unzClose(uf);
    return ok;
}

#else
#error "No zip backend selected. Configure with libzip or minizip."
#endif

} // namespace bik
