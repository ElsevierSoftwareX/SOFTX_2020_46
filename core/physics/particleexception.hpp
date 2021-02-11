/*!
 * gxsview version 1.2
 *
 * Copyright (c) 2020 Ohnishi Seiki and National Maritime Research Institute, Japan
 *
 * Released under the GPLv3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you need to distribute with another license,
 * ask ohnishi@m.mpat.go.jp
 */
#ifndef PARTICLEEXCEPTION_HPP
#define PARTICLEEXCEPTION_HPP

#include <exception>
#include <memory>
#include <string>


#include <sstream>


#include "core/math/nvector.hpp"

namespace geom {
class Cell;
}

namespace phys {

class ParticleException: public std::runtime_error {
public:
	explicit ParticleException(const char* message): std::runtime_error(message){;}
	ParticleException(const std::string &reason,
					  const geom::Cell *cell,
					  const math::Point &pt,
					  const math::Vector<3> &dir);

	virtual ~ParticleException() throw (){;}

	virtual const char *what() const throw();

	const std::string &reason() const;
	const geom::Cell  *cell() const;
	const math::Point &position() const;
	const math::Vector<3> &direction() const;

protected:
	std::string reason_;   // 発生理由
	const geom::Cell *cell_; // 発生したセル
	math::Point position_; // エラー発生場所
	math::Vector<3> direction_; // エラー発生時の粒子の方向

};


class InvalidSource: public ParticleException
{
public:
	InvalidSource(const std::string &reason,
					  const geom::Cell *cell,
					  const math::Point &pt,
					  const math::Vector<3> &dir);
};

class NoNewCell: public ParticleException
{
public:
	NoNewCell(const std::string &reason,
					  const geom::Cell  *cell,
					  const math::Point &pt,
					  const math::Vector<3> &dir);
};

class NoIntersection: public ParticleException
{
public:
	NoIntersection(const std::string &reason,
					  const geom::Cell *cell,
					  const math::Point &pt,
					  const math::Vector<3> &dir);
};


}  // end namespace phys

#endif // PARTICLEEXCEPTION_HPP
