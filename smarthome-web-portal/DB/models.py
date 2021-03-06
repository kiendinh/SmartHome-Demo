# -*- coding: utf-8 -*-
"""
Model definitions and its Mixins
"""
import datetime
import json
from sqlalchemy import *
# from sqlalchemy import event
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relation, backref
from sqlalchemy.dialects.mysql import DOUBLE

from DB import exception
from utils import util


Base = declarative_base()


class DefaultMixin(object):
    id = Column(Integer, primary_key=True)
    created_at = Column(DateTime, default=util.get_utc_now)


class JSONEncoded(TypeDecorator):
    """Represents an immutable structure as a json-encoded string."""

    impl = Text

    def process_bind_param(self, value, dialect):
        if value is not None:
            value = json.dumps(value)
        return value

    def process_result_value(self, value, dialect):
        if value is not None:
            value = json.loads(value)
        return value


class HelperMixin(object):
    def initialize(self):
        self.update()

    def update(self):
        pass

    @staticmethod
    def type_compatible(value, column_type):
        if value is None:
            return True
        if not hasattr(column_type, 'python_type'):
            return True
        column_python_type = column_type.python_type
        if isinstance(value, column_python_type):
            return True
        from decimal import Decimal
        if column_python_type in [Decimal]:
            return type(value) in [float, int]
        if issubclass(column_python_type, basestring):
            return isinstance(value, basestring)
        if column_python_type in [int, long]:
            return type(value) in [int, long]
        if column_python_type in [float]:
            return type(value) in [float, int]
        if column_python_type in [bool]:
            return type(value) in [bool]
        return False

    def validate(self):
        columns = self.__mapper__.columns
        for column in columns:
            value = getattr(self, column.name)
            if not self.type_compatible(value, column.type):
                raise exception.InvalidParameter(
                        'column %s value %r type is unexpected: %s' % (
                            column.name, value, column.type)
                )

    def to_dict(self):
        keys = self.__mapper__.columns.keys()
        dict_info = {}
        for key in keys:
            if key.startswith('_'):
                continue
            value = getattr(self, key)
            if value is not None:
                if isinstance(value, datetime.datetime):
                    value = util.format_datetime(value)
                dict_info[key] = value
        return dict_info

    def join_to_dict(self):
        joined = dict([(k, v) for k, v in self.__dict__.iteritems()
                       if not k[0] == '_'])
        dict_info = {}
        for key in joined.keys():
            if key.startswith('_'):
                continue
            value = getattr(self, key)
            if value is not None:
                if isinstance(value, datetime.datetime):
                    value = util.format_datetime(value)
                if isinstance(value, HelperMixin):
                    value = value.to_dict()
                dict_info[key] = value

        return dict_info

    def join_to_dict_recurse(self):
        joined = dict([(k, v) for k, v in self.__dict__.iteritems()
                       if not k[0] == '_'])
        dict_info = {}
        for key in joined.keys():
            if key.startswith('_'):
                continue
            value = getattr(self, key)
            if value is not None:
                if isinstance(value, datetime.datetime):
                    value = util.format_datetime(value)
                if isinstance(value, HelperMixin):
                    value = value.join_to_dict_recurse()
                dict_info[key] = value

        return dict_info


class User(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'user'

    username = Column(VARCHAR(20))
    password = Column(VARCHAR(64))
    phone = Column(VARCHAR(15))
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    gateway = relation('Gateway', backref='user', lazy=False)

    def __init__(self, username=None, password=None, gateway_id=None):
        self.username = username
        self.password = password
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<User(id='%s',username='%s',password='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.username, self.password, self.gateway_id, str(self.created_at))


class Fan(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'fan'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, status=None, gateway_id=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Fan(id='%s',uuid='%s',status='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.status), str(self.gateway_id), str(self.created_at))


class Button(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'button'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, status=None, gateway_id=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Button(id='%s',uuid='%s',status='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.status), str(self.gateway_id), str(self.created_at))


class Temperature(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'temperature'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    temperature = Column(DOUBLE(precision=20, scale=15))
    units = Column(VARCHAR(10))
    range = Column(VARCHAR(20))
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, temperature=None, units=None, range=None, gateway_id=None):
        self.temperature = temperature
        self.units = units
        self.range = range
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Temperature(id='%s',uuid='%s',temperature='%s',units='%s',range='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.temperature), str(self.units), str(self.range), str(self.gateway_id),
            str(self.created_at))


class Rgbled(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'rgbled'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    rgbvalue = Column(VARCHAR(20))
    range = Column(VARCHAR(20))
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, rgbvalue=None, range=None, gateway_id=None):
        self.rgbvalue = rgbvalue
        self.range = range
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Rgbled(id='%s',uuid='%s',rgbvalue='%s',range='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.rgbvalue), str(self.range), str(self.gateway_id), str(self.created_at))


class Led(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'led'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, status=None, gateway_id=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Led(id='%s',uuid='%s',status='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.status), str(self.gateway_id), str(self.created_at))


class Buzzer(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'buzzer'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, status=None, gateway_id=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Buzzer(id='%s',uuid='%s',status='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.status), str(self.gateway_id), str(self.created_at))


class Illuminance(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'illuminance'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    illuminance = Column(Float)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, illuminance=None, gateway_id=None):
        self.illuminance = illuminance
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Illuminance(id='%s',uuid='%s',illuminance='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.illuminance), str(self.gateway_id), str(self.created_at))


class Motion(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'motion'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, status=None, gateway_id=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Motion(id='%s',uuid='%s',status='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.status), str(self.gateway_id), str(self.created_at))


class Gas(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'gas'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, status=None, gateway_id=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Gas(id='%s',uuid='%s',status='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.status), str(self.gateway_id), str(self.created_at))


class Solar(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'solar'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    tiltpercentage = Column(Float)
    lcd_first = Column(VARCHAR(30))
    lcd_second = Column(VARCHAR(30))

    def __init__(self, uuid=None, status=None, gateway_id=None, tiltpercentage=None, lcd_first=None, lcd_second=None):
        self.status = status
        self.uuid = uuid
        self.gateway_id = gateway_id
        self.tiltpercentage = tiltpercentage
        self.lcd_first = lcd_first
        self.lcd_second = lcd_second

    def __repr__(self):
        return "<Solar(id='%s',uuid='%s',status='%s',gateway_id='%s',tiltpercentage='%s',lcd_first='%s'," \
               "lcd_second='%s',created_at='%s')>" % (str(self.id), self.uuid, str(self.status),
                                                      str(self.gateway_id), str(self.tiltpercentage),
                                                      self.lcd_first, str(self.lcd_second), str(self.created_at))


class Power(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'power'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    value = Column(Integer)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, value=None, gateway_id=None):
        self.value = value
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Power(id='%s',uuid='%s',value='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.value), str(self.gateway_id), str(self.created_at))


class Energy(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'energy'

    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    value = Column(Integer)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, value=None, gateway_id=None):
        self.value = value
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<Energy(id='%s',uuid='%s',value='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.value), str(self.gateway_id), str(self.created_at))


class EventLog(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'eventlog'

    type = Column(VARCHAR(10))
    data = Column(VARCHAR(100))
    response_code = Column(Integer)

    def __init__(self, type=None, data=None, response_code=None):
        self.type = type
        self.data = data
        self.response_code = response_code

    def __repr__(self):
        return "<EventLog(id='%s',type='%s',data='%s',response_code='%s',created_at='%s')>" % (
            str(self.id), self.type, self.data, str(self.response_code), str(self.created_at))


class SmsHistroy(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'sms_history'

    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    uuid = Column(VARCHAR(40), ForeignKey('resource.uuid', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)

    def __init__(self, uuid=None, gateway_id=None):
        self.uuid = uuid
        self.gateway_id = gateway_id

    def __repr__(self):
        return "<SMS History(id='%s',uuid='%s',gateway_id='%s',created_at='%s')>" % (
            str(self.id), self.uuid, str(self.gateway_id), str(self.created_at))


class Gateway(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'gateway'

    name = Column(VARCHAR(30))
    url = Column(VARCHAR(30))
    address = Column(VARCHAR(50))
    latitude = Column(VARCHAR(20))
    longitude = Column(VARCHAR(20))
    status = Column(Boolean)

    def __init__(self, name=None, url=None, address=None, latitude=None, longitude=None, status=False):
        self.name = name
        self.url = url
        self.address = address
        self.latitude = latitude
        self.longitude = longitude
        self.status = status

    def __repr__(self):
        return "<Gateway(id='%s',name='%s',url='%s',address='%s',latitude='%s', longitude='%s', created_at='%s')>" % (
            str(self.id), self.name, self.url, self.address, self.latitude, self.longitude, str(self.created_at))


class Resource(Base, HelperMixin, DefaultMixin):
    __tablename__ = 'resource'

    uuid = Column(VARCHAR(40), nullable=False, index=True)
    sensor_type_id = Column(Integer, ForeignKey('sensor_type.id', ondelete='CASCADE', onupdate='CASCADE'),
                            nullable=False)
    status = Column(Boolean)
    gateway_id = Column(Integer, ForeignKey('gateway.id', ondelete='CASCADE', onupdate='CASCADE'), nullable=False)
    path = Column(VARCHAR(60), nullable=False)
    sensor_type = relation('SensorType', backref='resource', lazy=False)

    def __init__(self, uuid=None, sensor_type_id=None, path=None, status=None):
        self.uuid = uuid
        self.sensor_type_id = sensor_type_id
        self.path = path
        self.status = status

    def __repr__(self):
        return "<Resource(id='%s', path='%s', status='%s', created_at='%s')>" % (
            str(self.id), self.path, self.status, str(self.created_at))


class SensorType(Base, HelperMixin):
    __tablename__ = 'sensor_type'
    id = Column(Integer, primary_key=True)
    type = Column(VARCHAR(20), nullable=False)

    def __init__(self, sensor_type=None):
        self.type = sensor_type

    def __repr__(self):
        return "<SensorType(id='%s', type='%s')>" % (str(self.id), self.type)

# if __name__ == '__main__':
#    initial_db()
