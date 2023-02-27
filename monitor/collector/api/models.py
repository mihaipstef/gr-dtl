import datetime
from sqlalchemy import (
    Column, Integer, String, DateTime, Float, Boolean, ForeignKey)
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.inspection import inspect
import uuid

Base = declarative_base()


class MonitorData(Base):

    __tablename__ = "monitor_data"

    id = Column(String, primary_key=True, default=lambda: str(uuid.uuid4()))
    pair_id = Column(ForeignKey("subscribers.pair_id"))
    date_created = Column(
        DateTime, default=datetime.datetime.utcnow, nullable=False)
    date_updated = Column(
        DateTime, default=datetime.datetime.utcnow, onupdate=datetime.datetime.utcnow, nullable=False)
    tx_frame_count = Column(Integer, nullable=True)
    rx_frame_count = Column(Integer, nullable=True)
    payload_crc_success = Column(Integer, nullable=True)
    payload_crc_failed = Column(Integer, nullable=True)
    header_crc_success = Column(Integer, nullable=True)
    header_crc_failed = Column(Integer, nullable=True)
    snr_est = Column(Float, nullable=True)
    current_bps = Column(Integer, nullable=True)

    def update(self, **kwargs):
        for k, v in kwargs.items():
            if hasattr(self, k):
                setattr(self, k, v)

    def to_dict(self):
        return {c: getattr(self, c) for c in inspect(self).attrs.keys()}


class Subscribers(Base):

    __tablename__ = "subscribers"

    pair_id = Column(String, primary_key=True, nullable=False)
    date_created = Column(
        DateTime, default=datetime.datetime.utcnow, nullable=False)
    date_updated = Column(
        DateTime, default=datetime.datetime.utcnow, onupdate=datetime.datetime.utcnow, nullable=False)
    is_active = Column(Boolean, nullable=False)
    rx_sub_id = Column(String, nullable=True)
    tx_sub_id = Column(String, nullable=True)
    description = Column(String, nullable=True)

    def update(self, **kwargs):
        for k, v in kwargs.items():
            if hasattr(self, k):
                setattr(self, k, v)

    def to_dict(self):
        return {c: getattr(self, c) for c in inspect(self).attrs.keys()}