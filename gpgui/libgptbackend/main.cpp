/*
 * GPGUI - Group Policy Editor GUI
 *
 * Copyright (C) 2020 BaseALT Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "registry.h"

#include "preg_data.h"

#include <string>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

const char *regtype2str(uint32_t &regtype) {
    return preg::regtype2str(regtype).c_str();
}

uint32_t str2regtype(const char *regtype) {
    std::string reg_name(regtype);
    return preg::str2regtype(reg_name);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
