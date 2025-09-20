/*
 Copyright (C) 2022-2023 Fredrik Öhrström (gpl-3.0-or-later)
 Copyright (C) 2022 Kajetan Krykwiński (gpl-3.0-or-later)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"meters_common_implementation.h"

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);

        void processContent(Telegram *t);
        std::string dateToString(uchar date_lo, uchar date_hi);
    };

    static bool ok = registerDriver([](DriverInfo&di)
    {
        // Note: this supports only E.ITN 30.51 at the moment.
        // E.ITN 30.60 should be similar, as it is covered via the same datasheet
        // http://www.apator.com/uploads/files/Produkty/Podzielnik_kosztow_ogrzewania/i-pl-021-2016-e-itn-30-51-30-6.pdf
        di.setName("apatoreitn40");
        di.setDefaultFields("name,id,current_hca,previous_hca,current_date,season_start_date,esb_date,temp_room_avg_c,temp_room_prev_avg_c,timestamp");
        di.setMeterType(MeterType::HeatCostAllocationMeter);
        di.addDetection(MANUFACTURER_APA, 0x08,  0x04);
        di.addDetection(MANUFACTURER_APT, 0x08,  0x04);
        di.addDetection(0x8614 /* APT? */, 0x08,  0x04);
        di.addDetection(0x8601 /* APA? */, 0x08,  0x04);
        di.usesProcessContent();
        di.setConstructor([](MeterInfo& mi, DriverInfo& di){ return shared_ptr<Meter>(new Driver(mi, di)); });
    });

    Driver::Driver(MeterInfo &mi, DriverInfo &di) : MeterCommonImplementation(mi, di)
    {
        addNumericField("current",
                 Quantity::HCA,
                 DEFAULT_PRINT_PROPERTIES,
                 "Energy consumption so far in this billing period.");

        addNumericField("previous",
                 Quantity::HCA,
                 DEFAULT_PRINT_PROPERTIES,
                 "Energy consumption in previous billing period.");

        addStringField("current_date",
                       "Current date, as reported by meter.",
                       DEFAULT_PRINT_PROPERTIES);

        addStringField("season_start_date",
                       "Season start date.",
                       DEFAULT_PRINT_PROPERTIES);

        addStringField("esb_date",
                       "Electronic seal protection break date.",
                       DEFAULT_PRINT_PROPERTIES);

        addNumericField("temp_room_avg",
                 Quantity::Temperature,
                 DEFAULT_PRINT_PROPERTIES,
                 "Average room temperature in current season.");

        addNumericField("temp_room_prev_avg",
                 Quantity::Temperature,
                 DEFAULT_PRINT_PROPERTIES,
                 "Average room temperature in previous season.");
    }

    void Driver::processContent(Telegram *t)
    {
        std::vector<uchar> content;
        t->extractPayload(&content);

        // if (t->tpl_ci == 0xB6) {
        //     // tpl-ci-field B6, there's a header to skip
        //     // first byte contains following header length
        //     uint header_len = content[0] + 1;

        //     //drop header data from content
        //     content.erase(content.begin(), content.begin() + header_len);
        // }

        if (t->tpl_ci == 0xA0) {
            // In my opinion this is already part of the telegram data
            // so lets add it back to content.
            // Telegram either starts with B0<hdr_len><hdr>A0A1...
            // or directly with A0A1...
            content.insert(content.begin(),t->tpl_ci);
        }

        // if (content.size() != 16)
        // {
        //     // Payload most likely is broken
        //     debugPayload("(apatoreitn) content size wrong!", content);
        //     return;
        // }

        // Season start date + install date + some flag?
        //
        // Note: may be wrong, requires confirmation as all meters I see in range
        //       report start date 1.05, installed in 2016 and field is A0A1h
        // Note: NOT byte swapped. Accidentally? works via dateToString conversion.
        // uchar season_start_date_lo = content[1];
        // uchar season_start_date_hi = content[0];
        // std::string season_start_date = dateToString(season_start_date_lo, season_start_date_hi);
        // setStringValue("season_start_date", season_start_date, NULL);

        // Previous season total allocation
        uchar prev_lo = content[4];
        uchar prev_hi = content[5];
        double previous_hca = (256.0*prev_hi+prev_lo);
        setNumericValue("previous", Unit::HCA, previous_hca);

        // // Electronic seal break date
        // uchar esb_date_lo = content[6];
        // uchar esb_date_hi = content[7];
        // string esb_date = dateToString(esb_date_lo, esb_date_hi);
        // setStringValue("esb_date", esb_date, NULL);

        // Current season allocation
        uchar curr_lo = content[9];
        uchar curr_hi = content[10];
        double current_hca = (256.0*curr_hi+curr_lo);
        setNumericValue("current", Unit::HCA, current_hca);

        // Current date reported by meter
        // uchar date_curr_lo = content[10];
        // uchar date_curr_hi = content[11];
        // string current_date = dateToString(date_curr_lo, date_curr_hi);
        // setStringValue("current_date", current_date, NULL);

        // Previous season average temperature
        double temp_room_prev_avg_frac = content[41];
        double temp_room_prev_avg_deg = content[42];
        double temp_room_prev_avg = temp_room_prev_avg_deg + temp_room_prev_avg_frac/256.0;
        setNumericValue("temp_room_prev_avg", Unit::C, temp_room_prev_avg);

        // Current season average temperature
        // double temp_room_avg_frac = content[14];
        // double temp_room_avg_deg = content[15];
        // double temp_room_avg = temp_room_avg_deg + temp_room_avg_frac/256.0;
        // setNumericValue("temp_room_avg", Unit::C, temp_room_avg);
    }

    string Driver::dateToString(uchar date_lo, uchar date_hi) {
        // Data format (MSB -> LSB):
        // 2 bits of unknown data (or part of a year, but left over for season date
        //                         hack, and it doesn't matter until 2064 anyway...)
        // 5 bits of year
        // 4 bits of month
        // 5 bits of day
        uint date_curr = (256*date_hi + date_lo);
        if(date_curr == 0)
        {
            // date is null, what should we do?
            return "";
        }

        uint day = date_curr & 0x1F;
        uint month = (date_curr >> 5) & 0x0F;
        uint year = ((date_curr >> 9) & 0x1F) + 2000;

        char buf[30];
        // is there a better way to construct a date from this?
        std::snprintf(buf, sizeof(buf), "%d-%02d-%02dT02:00:00Z", year, month, day);
        return buf;
    }
}
