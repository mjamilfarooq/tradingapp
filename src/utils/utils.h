/*
 * utils.h
 *
 *  Created on: Nov 28, 2016
 *      Author: jamil
 */

#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <memory>


/*
 * @brief since c++11 doesn't have make_unique utility like make_shared
 * for exception safety I have implemented it on my own
 *
 * @param all the paramters which is require to pass to the constructor of Type T
 *
 * @return std::unique_ptr of type T
 */
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

/*
 * @brief truncate for decimal number up to given decimal places
 *
 * @param number to apply truncate to
 *
 * @param digits to which truncation is applied to
 *
 * @return truncated number
 */
inline double truncate(double number, uint32_t digits) {
	return int32_t(number*pow(10, digits))/pow(10, digits);
}

/*
 * @brief ceiling for decimal number up to given decimal places
 *
 * @param number to apply ceil to
 *
 * @param digits to which ceiling is applied to
 *
 * @return ceiled number
 */
inline double dceil(double number, uint32_t digits) {
	auto factor = pow(10.0, digits);
	auto d = number*factor;
	auto n = uint32_t(d);
	auto mod = d - n;
	if ( mod > 0 ) n++;
	return n/factor;
}

/*
 * @brief flooring for decimal number up to given decimal places. this is same as truncate
 *
 * @param number to apply floor to
 *
 * @param digits to which flooring is applied to
 *
 * @return floored number
 */
inline double dfloor(double number, uint32_t digits) {
	auto factor = pow(10.0, digits);
	auto d = number*factor;
	auto n = uint32_t(d);
//	auto mod = d - n;
//	if ( mod > 0 ) n++;
	return n/factor;
}
#endif /* SRC_UTILS_UTILS_H_ */
