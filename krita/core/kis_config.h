/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#if !defined KIS_CONFIG_H_
#define KIS_CONFIG_H_

class KisConfig {
public:
	KisConfig();
	~KisConfig();

	Q_INT32 maxImgWidth() const;
	Q_INT32 defImgWidth() const;
	Q_INT32 maxImgHeight() const;
	Q_INT32 defImgHeight() const;
	Q_INT32 maxLayerWidth() const;
	Q_INT32 defLayerWidth() const;
	Q_INT32 maxLayerHeight() const;
	Q_INT32 defLayerHeight() const;

private:
	KisConfig(const KisConfig&);
	KisConfig& operator=(const KisConfig&);

private:
	mutable KConfig *m_cfg;
};

#endif // KIS_CONFIG_H_

