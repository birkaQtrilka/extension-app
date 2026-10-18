//
// Created by Birka on 18/10/2026.
//

#ifndef IFILEPROBE_H
#define IFILEPROBE_H
class IFileProbe {
public:
    virtual ~IFileProbe() = default;
    virtual bool isStable(const std::filesystem::path& path) const = 0;
    virtual  bool isRegularFile(const std::filesystem::path& path) const = 0;
};
#endif //IFILEPROBE_H
