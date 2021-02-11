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
#include "particleexception.hpp"

#include "core/geometry/cell/cell.hpp"

phys::ParticleException::ParticleException(const std::string &reason,
										   const geom::Cell  *cell,
										   const math::Point &pt,
										   const math::Vector<3> &dir)
	:std::runtime_error(reason + ", "
						+ (cell ? ("cell:" + cell->cellName()) : "")
						+ ", pos{" +pt.toString() + "}, dir{" + dir.toString() + "}"),
	reason_(reason), cell_(cell), position_(pt), direction_(dir)
{;}

const char *phys::ParticleException::what() const throw()
{
	return std::runtime_error::what();
}

const std::string &phys::ParticleException::reason() const {return reason_;}

const geom::Cell  *phys::ParticleException::cell() const {return cell_;}

const math::Point &phys::ParticleException::position() const {return position_;}

const math::Vector<3> &phys::ParticleException::direction() const {return direction_;}


phys::InvalidSource::InvalidSource(const std::string &reason, const geom::Cell *cell, const math::Point &pt, const math::Vector<3> &dir)
	: ParticleException(reason, cell, pt, dir)
{;}

phys::NoNewCell::NoNewCell(const std::string &reason, const geom::Cell *cell, const math::Point &pt, const math::Vector<3> &dir)
	: ParticleException(reason, cell, pt, dir)
{;}

phys::NoIntersection::NoIntersection(const std::string &reason, const geom::Cell *cell, const math::Point &pt, const math::Vector<3> &dir)
	: ParticleException(reason, cell, pt, dir)
{;}

