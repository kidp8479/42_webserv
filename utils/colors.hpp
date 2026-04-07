#ifndef COLORS_HPP
#define COLORS_HPP

#include <string>

/* =========================
   ANSI Color Macros
   ========================= */

#define BLK      "\033[30m"
#define RED      "\033[31m"
#define GRN      "\033[32m"
#define YEL      "\033[33m"
#define BLU      "\033[34m"
#define MAG      "\033[35m"
#define CYN      "\033[36m"
#define WHT      "\033[37m"

#define BR_BLK   "\033[1;30m"
#define BR_RED   "\033[1;31m"
#define BR_GRN   "\033[1;32m"
#define BR_YEL   "\033[1;33m"
#define BR_BLU   "\033[1;34m"
#define BR_MAG   "\033[1;35m"
#define BR_CYN   "\033[1;36m"
#define BR_WHT   "\033[1;37m"

#define RESET    "\033[0m"

/* =========================
   Wrapper functions
   ========================= */

inline std::string blk(const std::string& msg) { return std::string(BLK) + msg + RESET; }
inline std::string red(const std::string& msg) { return std::string(RED) + msg + RESET; }
inline std::string grn(const std::string& msg) { return std::string(GRN) + msg + RESET; }
inline std::string yel(const std::string& msg) { return std::string(YEL) + msg + RESET; }
inline std::string blu(const std::string& msg) { return std::string(BLU) + msg + RESET; }
inline std::string mag(const std::string& msg) { return std::string(MAG) + msg + RESET; }
inline std::string cyn(const std::string& msg) { return std::string(CYN) + msg + RESET; }
inline std::string wht(const std::string& msg) { return std::string(WHT) + msg + RESET; }

inline std::string brBlk(const std::string& msg) { return std::string(BR_BLK) + msg + RESET; }
inline std::string brRed(const std::string& msg) { return std::string(BR_RED) + msg + RESET; }
inline std::string brGrn(const std::string& msg) { return std::string(BR_GRN) + msg + RESET; }
inline std::string brYel(const std::string& msg) { return std::string(BR_YEL) + msg + RESET; }
inline std::string brBlu(const std::string& msg) { return std::string(BR_BLU) + msg + RESET; }
inline std::string brMag(const std::string& msg) { return std::string(BR_MAG) + msg + RESET; }
inline std::string brCyn(const std::string& msg) { return std::string(BR_CYN) + msg + RESET; }
inline std::string brWht(const std::string& msg) { return std::string(BR_WHT) + msg + RESET; }

#endif
